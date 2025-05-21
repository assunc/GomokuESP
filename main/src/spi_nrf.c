/*
    This code demonstrates how to use the SPI master half duplex mode to read/write a AT932C46D
    nrf (8-bit mode).

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// Extended by the GAME2 Team
/**
 * @title NRF24L01 Transceiver Application
 * @file spi_nrf.c
 * @Generate by ChatGPT
 * @Modified by FeiYang Zheng(GAME2)
 * @version 0.1
 * @date 2025-04-24
 * 
 */

#include <string.h>
#include "spi_nrf.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <unistd.h>
#include "esp_log.h"
#include <sys/param.h>

#define NRF_BUSY_TIMEOUT_MS  5

#define NRF_CLK_FREQ         (1000000)   //When powered by 3.3V, nrf max freq is 1MHz
#define NRF_INPUT_DELAY_NS   ((1000000000/NRF_CLK_FREQ)/2+20)

#define ADDR_MASK   0x7f

#define CMD_EWDS    0x200
#define CMD_WRAL    0x200
#define CMD_ERAL    0x200
#define CMD_EWEN    0x200
#define CMD_CKBS    0x000
#define CMD_READ    0x300
#define CMD_ERASE   0x380
#define CMD_WRITE   0x280

#define ADD_EWDS    0x00
#define ADD_WRAL    0x20
#define ADD_ERAL    0x40
#define ADD_EWEN    0x60

spi_device_handle_t spi;

void nrf_gpio_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << NRF_CE_PIN) | (1ULL << NRF_CSN_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_level(NRF_CE_PIN, 0);   // make ce low by default
    gpio_set_level(NRF_CSN_PIN, 1); // make csn high by default
}

void nrf_write_register(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2] = { NRF_CMD_W_REGISTER | reg, value };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // make CSN low to strat transmiting data
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // make CSN low to end transmision
}

void nrf_write_register_multi(uint8_t reg, uint8_t *data, size_t length) {
    uint8_t tx_data[length + 1];
    tx_data[0] = NRF_CMD_W_REGISTER | reg;
    memcpy(&tx_data[1], data, length);
    spi_transaction_t t = {
        .length = (length + 1) * 8,
        .tx_buffer = tx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // make CSN low to strat transmiting data
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); // make CSN low to end transmision
}

void nrf_write_payload(uint8_t *data, size_t length) {
    uint8_t cmd = NRF_CMD_W_TX_PAYLOAD;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // make CSN low to strat transmiting data
    spi_device_transmit(spi, &t);  //  transmit the send payload instruction
    t.length = length * 8;
    t.tx_buffer = data;
    spi_device_transmit(spi, &t);  // send data to another nrf24
    gpio_set_level(NRF_CSN_PIN, 1); // make CSN low to end transmision
}

void nrf_read_payload(uint8_t *data, size_t length) {
    uint8_t cmd = NRF_CMD_R_RX_PAYLOAD;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    gpio_set_level(NRF_CSN_PIN, 0); // make CSN low to strat transmiting data
    spi_device_transmit(spi, &t);  // transmit the send payload instruction
    t.length = length * 8;
    t.tx_buffer = NULL;
    t.rx_buffer = data;
    spi_device_transmit(spi, &t);  // read data from nrf24
    gpio_set_level(NRF_CSN_PIN, 1); // make CSN low to end transmision
}

uint8_t nrf_read_register(uint8_t reg) {
    uint8_t tx_data[2] = { NRF_CMD_R_REGISTER | reg, 0xFF };
    uint8_t rx_data[2] = {0};
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    gpio_set_level(NRF_CSN_PIN, 0); 
    spi_device_transmit(spi, &t);
    gpio_set_level(NRF_CSN_PIN, 1); 
    return rx_data[1];
}

void nrf_read_register_multi(uint8_t reg, uint8_t *data, size_t length) {
    uint8_t tx_data = NRF_CMD_R_REGISTER | reg; // do or operation to read register command code and register address
    spi_transaction_t t = {
        .length = 8,           
        .tx_buffer = &tx_data, 
    };

    gpio_set_level(NRF_CSN_PIN, 0); 
    spi_device_transmit(spi, &t);  

    t.length = length * 8;  // prepare to receive length bytes's data
    t.tx_buffer = NULL;    
    t.rx_buffer = data;     
    spi_device_transmit(spi, &t);  
    gpio_set_level(NRF_CSN_PIN, 1); 
}

void spi_init() {
    spi_bus_config_t buscfg = {
        .miso_io_num = GPIO_NUM_11,
        .mosi_io_num = GPIO_NUM_12,
        .sclk_io_num = GPIO_NUM_13,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0, 
        .spics_io_num = -1,//< CS GPIO pin for this device, or -1 if not used
        .queue_size = 7,
    };

    spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI_HOST, &devcfg, &spi);
}

void nrf_init() {
    nrf_gpio_init(); // configure GPIO
    spi_init();      // configure SPI

    nrf_write_register(NRF_REG_CONFIG, 0x3F);   //enable interrupt for receiving. enable CRC, and set to receive mode
    nrf_write_register(NRF_REG_RF_SETUP, 0x06); // set RF output power to 0 dbm
    nrf_write_register(NRF_REG_RF_CH, 0x10);    // set RF frequency 0x61

    // uint8_t tx_address[5] = { 0xE7, 0xC9, 0x3F, 0x03, 0x00 }; // 
    uint8_t tx_address[5] = { 0x00, 0x00, 0x00, 0x00, 0x01 }; // set tx address
    nrf_write_register_multi(NRF_REG_TX_ADDR, tx_address, 5);
    nrf_write_register_multi(NRF_REG_RX_ADDR_P0, tx_address, 5); // set rx0 address to the same as transmit address for auto acknowledgement

    nrf_write_register(NRF_REG_EN_AA, 0x01); // enable auto acknowledgement for pipe 0
    nrf_write_register(NRF_REG_RX_PW_P0, 0x20); // receive byte is 32 bytes
}

void nrf_clear_tx_rx(void) {
    nrf_write_register(NRF_CMD_FLUSH_RX, 0);
    nrf_write_register(NRF_CMD_FLUSH_TX, 0);
}

void nrf_clear_interrupts() {
    nrf_write_register(NRF_REG_STATUS, 0x7E);
}

void nrf_send_data(uint8_t *data, size_t length) {
    while (1) {//continuously send data
        uint8_t full_data[32];
        memset(full_data, 0, sizeof(full_data)); // configure the sending buffer to 0
        memcpy(full_data, data, length);         // copy data 

        nrf_write_register(NRF_REG_CONFIG, 0x3E);        // to TX mode
        gpio_set_level(NRF_CE_PIN, 0);                   // Pull up CE to transmit; Mask TX_DS and MAX_RT
        nrf_write_payload(full_data, sizeof(full_data)); // write 32 bytes payload

        gpio_set_level(NRF_CE_PIN, 1); 
        vTaskDelay(1);                 
        gpio_set_level(NRF_CE_PIN, 0); 
        uint8_t status = 0;
        status = nrf_read_register(NRF_REG_STATUS);
        printf("STATUS: 0x%02X\n", status);

        if (status & (1 << 5)) { //check whether the data sent successfully. when there is data in its buffer, the interrupt flag will be set to high
            printf("Data sent successfully!\n");
            nrf_write_register(NRF_REG_CONFIG, 0x3F); // to RX mode
            gpio_set_level(NRF_CE_PIN, 1);
            nrf_write_register(NRF_REG_STATUS, 0x70);
            nrf_clear_tx_rx();
            break;
        } else if (status & (1 << 4)) { //reach the max resend time
            printf("Max retransmission reached. Flushing TX.\n");
            nrf_write_register(NRF_CMD_FLUSH_TX, 0); // flush send queue
        } else {
            printf("Data not sent :(\n");
        }

        nrf_write_register(NRF_REG_STATUS, 0x70);
        nrf_clear_tx_rx();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void nrf_check_configuration(void) {
    printf("=== Checking nRF24L01 Configuration ===\n");

    uint8_t config = nrf_read_register(NRF_REG_CONFIG);
    printf("CONFIG: 0x%02X\n", config);
    if ((config & 0x0F) != 0x0E && (config & 0x0F) != 0x0F) {
        printf("ERROR: CONFIG register is not properly configured (0x%02X)\n", config);
    } else {
        printf("CONFIG register is properly configured.\n");
    }

    uint8_t en_aa = nrf_read_register(NRF_REG_EN_AA);
    printf("EN_AA: 0x%02X (Auto Acknowledgment)\n", en_aa);

    uint8_t rf_setup = nrf_read_register(NRF_REG_RF_SETUP);
    printf("RF_SETUP: 0x%02X (RF settings)\n", rf_setup);

    uint8_t rf_ch = nrf_read_register(NRF_REG_RF_CH);
    printf("RF_CH: 0x%02X (RF channel)\n", rf_ch);

    uint8_t status = nrf_read_register(NRF_REG_STATUS);
    printf("STATUS: 0x%02X\n", status);

    uint8_t rx_address[5];
    nrf_read_register_multi(NRF_REG_RX_ADDR_P0, rx_address, 5);
    printf("RX_ADDR_P0: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", rx_address[i]);
    }
    printf("\n");

    uint8_t tx_address[5];
    nrf_read_register_multi(NRF_REG_TX_ADDR, tx_address, 5);
    printf("TX_ADDR: ");
    for (int i = 0; i < 5; i++) {
        printf("%02X ", tx_address[i]);
    }
    printf("\n");

    printf("=== Configuration Check Complete ===\n");
}