#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    spi_device_handle_t sci_handle;  // SPI device for SCI (control)
    spi_device_handle_t sdi_handle;  // SPI device for SDI (data)
    gpio_num_t dreq_pin;
    gpio_num_t reset_pin;
    bool initialized;
} vs1053_t;

// Initialization and control
esp_err_t vs1053_init(vs1053_t* vs, spi_host_device_t spi_host);
esp_err_t vs1053_reset(vs1053_t* vs);
esp_err_t vs1053_soft_reset(vs1053_t* vs);
esp_err_t vs1053_set_clock(vs1053_t* vs, uint16_t clockf);
esp_err_t vs1053_set_volume(vs1053_t* vs, uint8_t left, uint8_t right);

// DREQ handling
bool vs1053_dreq_ready(vs1053_t* vs);
void vs1053_wait_dreq(vs1053_t* vs);

// SCI register access
esp_err_t vs1053_sci_write(vs1053_t* vs, uint8_t reg, uint16_t value);
esp_err_t vs1053_sci_read(vs1053_t* vs, uint8_t reg, uint16_t* value);

// SDI data transfer
esp_err_t vs1053_sdi_write(vs1053_t* vs, const uint8_t* data, size_t len);

// ADPCM recording mode
esp_err_t vs1053_start_adpcm_record(vs1053_t* vs, uint32_t sample_rate, bool stereo);
esp_err_t vs1053_stop_record(vs1053_t* vs);
int vs1053_read_adpcm_block(vs1053_t* vs, uint8_t* buffer, size_t buffer_size);

// ADPCM playback mode
esp_err_t vs1053_start_adpcm_decode(vs1053_t* vs);
esp_err_t vs1053_write_wav_header_adpcm(vs1053_t* vs, uint32_t sample_rate, uint8_t channels);
esp_err_t vs1053_stop_playback(vs1053_t* vs);

#ifdef __cplusplus
}
#endif
