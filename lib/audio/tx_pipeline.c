#include "tx_pipeline.h"
#include "packet.h"
#include "config/build.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_timer.h"
#include <string.h>

static const char* TAG = "tx_pipeline";

static vs1053_t* s_vs = NULL;
static udp_handle_t s_udp = NULL;
static RingbufHandle_t s_ring_buffer = NULL;
static TaskHandle_t s_record_task = NULL;
static TaskHandle_t s_send_task = NULL;
static bool s_running = false;

static uint32_t s_blocks_produced = 0;
static uint32_t s_packets_sent = 0;
static uint32_t s_sequence = 0;

static void record_task(void* arg) {
    uint8_t block_buffer[ADPCM_BLOCK_SIZE_BYTES];
    
    ESP_LOGI(TAG, "Record task started");
    
    while (s_running) {
        int bytes_read = vs1053_read_adpcm_block(s_vs, block_buffer, sizeof(block_buffer));
        
        if (bytes_read == ADPCM_BLOCK_SIZE_BYTES) {
            if (xRingbufferSend(s_ring_buffer, block_buffer, bytes_read, pdMS_TO_TICKS(10)) == pdTRUE) {
                s_blocks_produced++;
            } else {
                ESP_LOGW(TAG, "Ring buffer full, dropping block");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    vTaskDelete(NULL);
}

static void send_task(void* arg) {
    audio_packet_t packet;
    size_t item_size;
    uint8_t* block_data;
    
    ESP_LOGI(TAG, "Send task started");
    
    while (s_running) {
        memset(&packet, 0, sizeof(packet));
        packet.seq = s_sequence++;
        packet.timestamp_us = esp_timer_get_time();
        
        int blocks_in_packet = 0;
        
        for (int i = 0; i < BLOCKS_PER_PACKET && s_running; i++) {
            block_data = xRingbufferReceive(s_ring_buffer, &item_size, pdMS_TO_TICKS(20));
            
            if (block_data != NULL && item_size == ADPCM_BLOCK_SIZE_BYTES) {
                memcpy(&packet.payload[i * ADPCM_BLOCK_SIZE_BYTES], block_data, ADPCM_BLOCK_SIZE_BYTES);
                vRingbufferReturnItem(s_ring_buffer, block_data);
                blocks_in_packet++;
            } else {
                if (block_data) vRingbufferReturnItem(s_ring_buffer, block_data);
                break;
            }
        }
        
        if (blocks_in_packet > 0) {
            // TODO: Send via UDP (s_udp)
            // For now, just count
            s_packets_sent++;
        }
    }
    
    vTaskDelete(NULL);
}

esp_err_t tx_pipeline_init(vs1053_t* vs, udp_handle_t udp) {
    if (s_running) {
        ESP_LOGE(TAG, "Pipeline already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    s_vs = vs;
    s_udp = udp;
    
    s_ring_buffer = xRingbufferCreate(TX_RING_BUFFER_BLOCKS * ADPCM_BLOCK_SIZE_BYTES, RINGBUF_TYPE_BYTEBUF);
    if (s_ring_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "TX pipeline initialized");
    return ESP_OK;
}

esp_err_t tx_pipeline_start(void) {
    if (s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = vs1053_start_adpcm_record(s_vs, AUDIO_SAMPLE_RATE, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start recording: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_running = true;
    
    xTaskCreate(record_task, "vs1053_record", 4096, NULL, 10, &s_record_task);
    xTaskCreate(send_task, "udp_send", 4096, NULL, 8, &s_send_task);
    
    ESP_LOGI(TAG, "TX pipeline started");
    return ESP_OK;
}

esp_err_t tx_pipeline_stop(void) {
    if (!s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    
    s_running = false;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    vs1053_stop_record(s_vs);
    
    ESP_LOGI(TAG, "TX pipeline stopped");
    return ESP_OK;
}

void tx_pipeline_get_stats(uint32_t* blocks_produced, uint32_t* packets_sent, uint32_t* buffer_fill) {
    if (blocks_produced) *blocks_produced = s_blocks_produced;
    if (packets_sent) *packets_sent = s_packets_sent;
    if (buffer_fill) {
        UBaseType_t waiting = 0;
        vRingbufferGetInfo(s_ring_buffer, NULL, NULL, NULL, NULL, &waiting);
        *buffer_fill = waiting / ADPCM_BLOCK_SIZE_BYTES;
    }
}
