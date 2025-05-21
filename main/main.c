/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
//
// main.c
// Extended by the GAME2 Team.
//
/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "spi_nrf.h"

#include "common.h"
#include "gap.h"
#include "gatt_svc.h"
#include "src/bot.h"
#include <time.h>

#define SAFETY_TASK_PERIOD (100 / portTICK_PERIOD_MS)

/* Library function declarations */
void ble_store_config_init(void);

/* Private function declarations */
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}

static void nimble_host_config_init(void) {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

// task started by interrupt received from nRF24
void safety_interrupt_handler() {
    /* Task entry log */
    printf("Interrupt triggered\n");
    uint8_t data = 0;
    nrf_read_payload(&data, 1);
    nrf_clear_interrupts();

    printf("data (from safety system interrupt): %d\n", data);
    /* Send if safety triggered */
    if (data) {
        send_safety_system_indication(data);
    }
    vTaskDelete(NULL);
}

// ISR from nRF24 interrupt
static void IRAM_ATTR safety_task() {
    xTaskCreate(safety_interrupt_handler, "safety_interrupt_handler", 41024, NULL, 5, NULL);
}

void app_main(void) {
    /* Local variables */
    int rc;
    esp_err_t ret;

    printf("%d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)); // shows space available for transposition table
    init_bot(15000);

    // /* Initialize SPI */
    nrf_init();
    nrf_check_configuration();

    // set up interrupt pin and ISR
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << NRF_IRQ_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(NRF_IRQ_PIN, safety_task, NULL);

    /*
     * NVS flash initialization
     * Dependency of BLE stack to store configurations
     */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    /* NimBLE stack initialization */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 ret);
        return;
    }

    /* GAP service initialization */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return;
    }

    /* GATT server initialization */
    rc = gatt_svc_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
        return;
    }

    /* NimBLE host configuration initialization */
    nimble_host_config_init();

    /* Start NimBLE host task thread and return */
    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);

    // srand(2);
    // Board* board = create_board();
    // char player = WHITE;
    // int x, y;
    // const clock_t start_time = clock();
    // while (check_winner(board) == '\0') {
    //     const int bot_move = bot_place_piece(board, player, 6);
    //     x = bot_move % BOARD_SIZE;
    //     y = bot_move / BOARD_SIZE;
    //     printf("%d %d\n", x, y);
    //     place_piece(board, x, y, player);
    //     print_board(board);
    //     player = player == WHITE ? BLACK : WHITE;
    // }
    // printf("time: %.3f\n", (double)(clock() - start_time) / CLOCKS_PER_SEC);

    return;
}
