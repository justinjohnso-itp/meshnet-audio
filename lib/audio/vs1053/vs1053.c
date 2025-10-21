#include "vs1053.h"
#include "vs1053_regs.h"
#include "config/pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "vs1053";

static esp_err_t spi_init_device(vs1053_t* vs, spi_host_device_t spi_host) {
    esp_err_t ret;
    
    spi_bus_config_t buscfg = {
        .miso_io_num = VS1053_SPI_MISO,
        .mosi_io_num = VS1053_SPI_MOSI,
        .sclk_io_num = VS1053_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    
    ret = spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    spi_device_interface_config_t sci_cfg = {
        .clock_speed_hz = 1 * 1000 * 1000,  // 1MHz for SCI
        .mode = 0,
        .spics_io_num = VS1053_XCS,
        .queue_size = 1,
        .flags = 0,
    };
    
    ret = spi_bus_add_device(spi_host, &sci_cfg, &vs->sci_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SCI device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    spi_device_interface_config_t sdi_cfg = {
        .clock_speed_hz = VS1053_SPI_FREQ_HZ,
        .mode = 0,
        .spics_io_num = VS1053_XDCS,
        .queue_size = 1,
        .flags = 0,
    };
    
    ret = spi_bus_add_device(spi_host, &sdi_cfg, &vs->sdi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SDI device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t vs1053_init(vs1053_t* vs, spi_host_device_t spi_host) {
    esp_err_t ret;
    
    memset(vs, 0, sizeof(vs1053_t));
    vs->dreq_pin = VS1053_DREQ;
    vs->reset_pin = VS1053_XRESET;
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << vs->dreq_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    io_conf.pin_bit_mask = (1ULL << vs->reset_pin);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    ret = spi_init_device(vs, spi_host);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = vs1053_reset(vs);
    if (ret != ESP_OK) {
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ret = vs1053_set_clock(vs, VS1053_CLOCKF_DEFAULT);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = vs1053_set_volume(vs, 0x20, 0x20);
    if (ret != ESP_OK) {
        return ret;
    }
    
    vs->initialized = true;
    ESP_LOGI(TAG, "VS1053 initialized successfully");
    return ESP_OK;
}

esp_err_t vs1053_reset(vs1053_t* vs) {
    gpio_set_level(vs->reset_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(vs->reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    return vs1053_soft_reset(vs);
}

esp_err_t vs1053_soft_reset(vs1053_t* vs) {
    esp_err_t ret = vs1053_sci_write(vs, VS1053_SCI_MODE, SM_SDINEW | SM_RESET);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return ret;
}

bool vs1053_dreq_ready(vs1053_t* vs) {
    return gpio_get_level(vs->dreq_pin) == 1;
}

void vs1053_wait_dreq(vs1053_t* vs) {
    int timeout = 1000;
    while (!vs1053_dreq_ready(vs) && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (timeout <= 0) {
        ESP_LOGW(TAG, "DREQ timeout");
    }
}

esp_err_t vs1053_sci_write(vs1053_t* vs, uint8_t reg, uint16_t value) {
    vs1053_wait_dreq(vs);
    
    uint8_t tx_data[4] = {
        0x02,
        reg,
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };
    
    spi_transaction_t trans = {
        .length = 32,
        .tx_buffer = tx_data,
        .rx_buffer = NULL,
    };
    
    return spi_device_transmit(vs->sci_handle, &trans);
}

esp_err_t vs1053_sci_read(vs1053_t* vs, uint8_t reg, uint16_t* value) {
    vs1053_wait_dreq(vs);
    
    uint8_t tx_data[4] = {0x03, reg, 0xFF, 0xFF};
    uint8_t rx_data[4] = {0};
    
    spi_transaction_t trans = {
        .length = 32,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };
    
    esp_err_t ret = spi_device_transmit(vs->sci_handle, &trans);
    if (ret == ESP_OK) {
        *value = (rx_data[2] << 8) | rx_data[3];
    }
    return ret;
}

esp_err_t vs1053_sdi_write(vs1053_t* vs, const uint8_t* data, size_t len) {
    const size_t chunk_size = 32;
    size_t offset = 0;
    
    while (offset < len) {
        vs1053_wait_dreq(vs);
        
        size_t to_send = (len - offset) > chunk_size ? chunk_size : (len - offset);
        
        spi_transaction_t trans = {
            .length = to_send * 8,
            .tx_buffer = data + offset,
            .rx_buffer = NULL,
        };
        
        esp_err_t ret = spi_device_transmit(vs->sdi_handle, &trans);
        if (ret != ESP_OK) {
            return ret;
        }
        
        offset += to_send;
    }
    
    return ESP_OK;
}

esp_err_t vs1053_set_clock(vs1053_t* vs, uint16_t clockf) {
    return vs1053_sci_write(vs, VS1053_SCI_CLOCKF, clockf);
}

esp_err_t vs1053_set_volume(vs1053_t* vs, uint8_t left, uint8_t right) {
    uint16_t vol = (left << 8) | right;
    return vs1053_sci_write(vs, VS1053_SCI_VOL, vol);
}

esp_err_t vs1053_start_adpcm_record(vs1053_t* vs, uint32_t sample_rate, bool stereo) {
    esp_err_t ret;
    
    uint16_t mode = SM_SDINEW | SM_ADPCM | SM_LINE1;
    ret = vs1053_sci_write(vs, VS1053_SCI_MODE, mode);
    if (ret != ESP_OK) return ret;
    
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL0, ADPCM_PROFILE_IMA);
    if (ret != ESP_OK) return ret;
    
    uint16_t sr_code = (sample_rate / 1000) & 0xFFFF;
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL1, sr_code);
    if (ret != ESP_OK) return ret;
    
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL3, stereo ? 1 : 0);
    if (ret != ESP_OK) return ret;
    
    ESP_LOGI(TAG, "ADPCM recording started: %luHz, %s", sample_rate, stereo ? "stereo" : "mono");
    return ESP_OK;
}

esp_err_t vs1053_stop_record(vs1053_t* vs) {
    return vs1053_soft_reset(vs);
}

int vs1053_read_adpcm_block(vs1053_t* vs, uint8_t* buffer, size_t buffer_size) {
    uint16_t hdat1;
    esp_err_t ret = vs1053_sci_read(vs, VS1053_SCI_HDAT1, &hdat1);
    if (ret != ESP_OK || hdat1 == 0) {
        return 0;
    }
    
    size_t bytes_to_read = hdat1 < buffer_size ? hdat1 : buffer_size;
    size_t bytes_read = 0;
    
    while (bytes_read < bytes_to_read) {
        uint16_t word;
        ret = vs1053_sci_read(vs, VS1053_SCI_HDAT0, &word);
        if (ret != ESP_OK) break;
        
        if (bytes_read < buffer_size) buffer[bytes_read++] = (uint8_t)(word >> 8);
        if (bytes_read < buffer_size) buffer[bytes_read++] = (uint8_t)(word & 0xFF);
    }
    
    return bytes_read;
}

esp_err_t vs1053_start_adpcm_decode(vs1053_t* vs) {
    uint16_t mode = SM_SDINEW;
    return vs1053_sci_write(vs, VS1053_SCI_MODE, mode);
}

esp_err_t vs1053_write_wav_header_adpcm(vs1053_t* vs, uint32_t sample_rate, uint8_t channels) {
    uint8_t wav_header[60];
    memset(wav_header, 0, sizeof(wav_header));
    
    uint16_t block_align = 256;
    uint16_t bits_per_sample = 4;
    
    // RIFF chunk
    memcpy(wav_header + 0, "RIFF", 4);
    *(uint32_t*)(wav_header + 4) = 0xFFFFFFFF;
    memcpy(wav_header + 8, "WAVE", 4);
    
    // fmt chunk
    memcpy(wav_header + 12, "fmt ", 4);
    *(uint32_t*)(wav_header + 16) = 20;
    *(uint16_t*)(wav_header + 20) = WAV_FORMAT_ADPCM;
    *(uint16_t*)(wav_header + 22) = channels;
    *(uint32_t*)(wav_header + 24) = sample_rate;
    *(uint32_t*)(wav_header + 28) = sample_rate * channels * bits_per_sample / 8;
    *(uint16_t*)(wav_header + 32) = block_align;
    *(uint16_t*)(wav_header + 34) = bits_per_sample;
    *(uint16_t*)(wav_header + 36) = 2;
    *(uint16_t*)(wav_header + 38) = 256;
    
    // data chunk
    memcpy(wav_header + 40, "data", 4);
    *(uint32_t*)(wav_header + 44) = 0xFFFFFFFF;
    
    return vs1053_sdi_write(vs, wav_header, 48);
}

esp_err_t vs1053_stop_playback(vs1053_t* vs) {
    return vs1053_soft_reset(vs);
}
