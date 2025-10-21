#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "vs1053.h"
#include "vs1053_regs.h"
#include "config/pins.h"
#include "esp_log.h"

#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_VS1053.h>

static const char* TAG = "vs1053_adafruit";

static Adafruit_VS1053* s_vs = nullptr;

static inline bool dreq_ready() {
    return s_vs ? s_vs->readyForData() : false;
}

static void wait_dreq() {
    int timeout = 1000;
    while (!dreq_ready() && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (timeout <= 0) {
        ESP_LOGW(TAG, "DREQ timeout");
    }
}

extern "C" {

esp_err_t vs1053_init(vs1053_t* vs, spi_host_device_t /*spi_host_unused*/) {
    if (!s_vs) {
        initArduino();
        
        SPI.begin(VS1053_SPI_SCK, VS1053_SPI_MISO, VS1053_SPI_MOSI);
        
        // Adafruit_VS1053 constructor: (rst, cs, dcs, dreq)
        static Adafruit_VS1053 vs_inst(VS1053_XRESET, VS1053_XCS, VS1053_XDCS, VS1053_DREQ);
        s_vs = &vs_inst;
        
        if (!s_vs->begin()) {
            ESP_LOGE(TAG, "Adafruit_VS1053 begin() failed");
            s_vs = nullptr;
            return ESP_FAIL;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
        
        vs1053_set_clock(vs, VS1053_CLOCKF_DEFAULT);
        vs1053_set_volume(vs, 0x20, 0x20);
        
        ESP_LOGI(TAG, "VS1053 initialized via Adafruit library");
    }
    
    if (vs) {
        vs->initialized = (s_vs != nullptr);
        vs->dreq_pin = (gpio_num_t)VS1053_DREQ;
        vs->reset_pin = (gpio_num_t)VS1053_XRESET;
    }
    return ESP_OK;
}

esp_err_t vs1053_reset(vs1053_t* /*vs*/) {
    if (!s_vs) return ESP_ERR_INVALID_STATE;
    s_vs->reset();
    vTaskDelay(pdMS_TO_TICKS(100));
    s_vs->softReset();
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

esp_err_t vs1053_soft_reset(vs1053_t* /*vs*/) {
    if (!s_vs) return ESP_ERR_INVALID_STATE;
    s_vs->softReset();
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_OK;
}

bool vs1053_dreq_ready(vs1053_t* /*vs*/) {
    return dreq_ready();
}

void vs1053_wait_dreq(vs1053_t* /*vs*/) {
    wait_dreq();
}

esp_err_t vs1053_sci_write(vs1053_t* /*vs*/, uint8_t reg, uint16_t value) {
    if (!s_vs) return ESP_ERR_INVALID_STATE;
    s_vs->sciWrite(reg, value);
    return ESP_OK;
}

esp_err_t vs1053_sci_read(vs1053_t* /*vs*/, uint8_t reg, uint16_t* value) {
    if (!s_vs || !value) return ESP_ERR_INVALID_ARG;
    *value = s_vs->sciRead(reg);
    return ESP_OK;
}

esp_err_t vs1053_sdi_write(vs1053_t* /*vs*/, const uint8_t* data, size_t len) {
    if (!s_vs || !data || !len) return ESP_ERR_INVALID_ARG;
    
    const size_t chunk = 32;
    size_t off = 0;
    while (off < len) {
        wait_dreq();
        size_t to_send = (len - off > chunk) ? chunk : (len - off);
        s_vs->playData(const_cast<uint8_t*>(data + off), to_send);
        off += to_send;
    }
    return ESP_OK;
}

esp_err_t vs1053_set_clock(vs1053_t* vs, uint16_t clockf) {
    return vs1053_sci_write(vs, VS1053_SCI_CLOCKF, clockf);
}

esp_err_t vs1053_set_volume(vs1053_t* vs, uint8_t left, uint8_t right) {
    uint16_t vol = ((uint16_t)left << 8) | right;
    return vs1053_sci_write(vs, VS1053_SCI_VOL, vol);
}

esp_err_t vs1053_start_adpcm_record(vs1053_t* vs, uint32_t sample_rate, bool stereo) {
    if (!s_vs) return ESP_ERR_INVALID_STATE;
    
    // Enter ADPCM record mode: SM_ADPCM MUST be set during reset
    uint16_t mode = SM_SDINEW | SM_RESET | SM_ADPCM;
    esp_err_t ret = vs1053_sci_write(vs, VS1053_SCI_MODE, mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter ADPCM record mode");
        return ret;
    }
    
    // Wait for DREQ to go high after reset
    wait_dreq();
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Configure AICTRL registers
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL0, 0);  // IMA ADPCM profile
    if (ret != ESP_OK) return ret;
    
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL1, (uint16_t)sample_rate);  // Sample rate in Hz
    if (ret != ESP_OK) return ret;
    
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL2, 0);  // Default AGC
    if (ret != ESP_OK) return ret;
    
    ret = vs1053_sci_write(vs, VS1053_SCI_AICTRL3, stereo ? 1 : 0);
    if (ret != ESP_OK) return ret;
    
    // Verify mode and read initial HDAT1
    uint16_t mode_check = 0, hdat1_check = 0;
    vs1053_sci_read(vs, VS1053_SCI_MODE, &mode_check);
    vTaskDelay(pdMS_TO_TICKS(100));
    vs1053_sci_read(vs, VS1053_SCI_HDAT1, &hdat1_check);
    
    ESP_LOGI(TAG, "ADPCM recording started: %" PRIu32 " Hz, %s", sample_rate, stereo ? "stereo" : "mono");
    ESP_LOGI(TAG, "  MODE=0x%04X, HDAT1=%u words", mode_check, hdat1_check);
    
    return ESP_OK;
}

esp_err_t vs1053_stop_record(vs1053_t* vs) {
    return vs1053_soft_reset(vs);
}

int vs1053_read_adpcm_block(vs1053_t* vs, uint8_t* buffer, size_t buffer_size) {
    if (!s_vs || !buffer || buffer_size == 0) return 0;
    
    // HDAT1 contains the number of 16-bit WORDS available, not bytes
    uint16_t words_available = 0;
    if (vs1053_sci_read(vs, VS1053_SCI_HDAT1, &words_available) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read HDAT1");
        return 0;
    }
    
    static uint32_t last_debug = 0;
    uint32_t now = xTaskGetTickCount() / configTICK_RATE_HZ;
    if (now - last_debug >= 5) {
        ESP_LOGI(TAG, "HDAT1 words_available=%u", words_available);
        last_debug = now;
    }
    
    if (words_available == 0) {
        return 0;
    }
    
    // Calculate how many words we can read (buffer_size is in bytes)
    size_t words_to_read = words_available;
    if (words_to_read * 2 > buffer_size) {
        words_to_read = buffer_size / 2;
    }
    
    // Read words from HDAT0
    size_t bytes_read = 0;
    for (size_t i = 0; i < words_to_read; i++) {
        uint16_t word = s_vs->sciRead(VS1053_SCI_HDAT0);
        buffer[bytes_read++] = (uint8_t)(word >> 8);
        buffer[bytes_read++] = (uint8_t)(word & 0xFF);
    }
    
    return (int)bytes_read;
}

esp_err_t vs1053_start_adpcm_decode(vs1053_t* vs) {
    return vs1053_sci_write(vs, VS1053_SCI_MODE, SM_SDINEW);
}

esp_err_t vs1053_write_wav_header_adpcm(vs1053_t* vs, uint32_t sample_rate, uint8_t channels) {
    uint8_t wav_header[60] = {0};
    const uint16_t block_align = 256;
    const uint16_t bits_per_sample = 4;
    
    memcpy(wav_header + 0,  "RIFF", 4);
    *(uint32_t*)(wav_header + 4) = 0xFFFFFFFF;
    memcpy(wav_header + 8,  "WAVE", 4);
    
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
    
    memcpy(wav_header + 40, "data", 4);
    *(uint32_t*)(wav_header + 44) = 0xFFFFFFFF;
    
    return vs1053_sdi_write(vs, wav_header, 48);
}

esp_err_t vs1053_stop_playback(vs1053_t* vs) {
    return vs1053_soft_reset(vs);
}

} // extern "C"
