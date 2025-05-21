/*
    This code demonstrates how to use the SPI master half duplex mode to read/write a AT932C46D
    nrf (8-bit mode).

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define SPI_HOST        SPI2_HOST
#define PIN_NUM_MISO    11
#define PIN_NUM_MOSI    12
#define PIN_NUM_CLK     13
#define NRF_CE_PIN      10
#define NRF_CSN_PIN     5
#define NRF_IRQ_PIN     6   
#define SPI_CLOCK_SPEED 1000000  //（1MHz）

// nRF24L01 register address
#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_RF_SETUP    0x06
#define NRF_REG_RF_CH       0x05
#define NRF_REG_STATUS      0x07
#define NRF_REG_TX_ADDR     0x10 //
#define NRF_REG_RX_ADDR_P0  0x0A
#define NRF_REG_RX_PW_P0    0x11 //
#define NRF_REG_FIFO_STATUS 0x17

// nRF24L01 command code
#define NRF_CMD_W_REGISTER  0x20
#define NRF_CMD_R_REGISTER  0x00
#define NRF_CMD_FLUSH_TX    0xE1
#define NRF_CMD_FLUSH_RX    0xE2
#define NRF_CMD_W_TX_PAYLOAD 0xA0
#define NRF_CMD_R_RX_PAYLOAD 0x61

/// Configurations of the spi_nrf
typedef struct {
    spi_host_device_t host; ///< The SPI host used, set before calling `spi_nrf_init()`
    gpio_num_t cs_io;       ///< CS gpio number, set before calling `spi_nrf_init()`
    gpio_num_t miso_io;     ///< MISO gpio number, set before calling `spi_nrf_init()`
    bool intr_used;         ///< Whether to use polling or interrupt when waiting for write to be done. Set before calling `spi_nrf_init()`.
} nrf_config_t;

typedef struct nrf_context_t* nrf_handle_t;

void nrf_write_register(uint8_t reg, uint8_t value);

void nrf_write_register_multi(uint8_t reg, uint8_t *data, size_t length);

void nrf_check_configuration(void);

void nrf_send_data(uint8_t *data, size_t length);

void nrf_clear_interrupts();

void nrf_init();

void spi_init();

uint8_t nrf_read_register(uint8_t reg);

void nrf_read_register_multi(uint8_t reg, uint8_t *data, size_t length);

void nrf_write_payload(uint8_t *data, size_t length);

void nrf_read_payload(uint8_t *data, size_t length);

void nrf_write_register(uint8_t reg, uint8_t value);

void nrf_gpio_init();
