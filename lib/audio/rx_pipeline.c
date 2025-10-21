#include "rx_pipeline.h"
#include "packet.h"
#include "config/build.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char* TAG = "rx_pipeline";

static vs1053_t* s_vs = NULL;
static udp_handle_t s_udp = NULL;
static QueueHandle_t s_jitter_buffer = NULL;
static TaskHandle_t s_playback_task = NULL;
static bool s_running = false;
static bool s_header_sent = false;

static uint32_t s_packets_received = 0;
static uint32_t s_packets_dropped = 0;
static uint32_t s_underruns = 0;

static void playback_task(void* arg) {
    audio_packet_t packet;
    
    ESP_LOGI(TAG, "Playback task started");
    
    while (s_running) {
        if (xQueueReceive(s_jitter_buffer, &packet, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (!s_header_sent) {
                esp_err_t ret = vs1053_write_wav_header_adpcm(s_vs, AUDIO_SAMPLE_RATE, AUDIO_CHANNELS);
                if (ret == ESP_OK) {
                    s_header_sent = true;
                    ESP_LOGI(TAG, "WAV header sent");
                } else {
                    ESP_LOGE(TAG, "Failed to send WAV header");
                    continue;
                }
            }
            
            for (int i = 0; i < BLOCKS_PER_PACKET; i++) {
                uint8_t* block = &packet.payload[i * ADPCM_BLOCK_SIZE_BYTES];
                esp_err_t ret = vs1053_sdi_write(s_vs, block, ADPCM_BLOCK_SIZE_BYTES);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to write block to VS1053");
                }
            }
        } else {
            s_underruns++;
            ESP_LOGD(TAG, "Jitter buffer underrun");
        }
    }
    
    vTaskDelete(NULL);
}

esp_err_t rx_pipeline_init(vs1053_t* vs, udp_handle_t udp) {
    if (s_running) {
        ESP_LOGE(TAG, "Pipeline already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    s_vs = vs;
    s_udp = udp;
    
    s_jitter_buffer = xQueueCreate(RX_JITTER_BUFFER_PKTS, sizeof(audio_packet_t));
    if (s_jitter_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to create jitter buffer");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "RX pipeline initialized");
    return ESP_OK;
}

esp_err_t rx_pipeline_start(void) {
    if (s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = vs1053_start_adpcm_decode(s_vs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start decode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_running = true;
    s_header_sent = false;
    
    xTaskCreate(playback_task, "vs1053_playback", 4096, NULL, 10, &s_playback_task);
    
    ESP_LOGI(TAG, "RX pipeline started");
    return ESP_OK;
}

esp_err_t rx_pipeline_stop(void) {
    if (!s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_running = false;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    vs1053_stop_playback(s_vs);
    
    ESP_LOGI(TAG, "RX pipeline stopped");
    return ESP_OK;
}

void rx_pipeline_get_stats(uint32_t* packets_received, uint32_t* packets_dropped, uint32_t* underruns) {
    if (packets_received) *packets_received = s_packets_received;
    if (packets_dropped) *packets_dropped = s_packets_dropped;
    if (underruns) *underruns = s_underruns;
}

// Called by network layer when packet arrives
void rx_pipeline_on_packet(const audio_packet_t* packet) {
    if (!s_running) return;
    
    s_packets_received++;
    
    if (xQueueSend(s_jitter_buffer, packet, 0) != pdTRUE) {
        s_packets_dropped++;
        ESP_LOGD(TAG, "Jitter buffer full, dropping packet");
    }
}
