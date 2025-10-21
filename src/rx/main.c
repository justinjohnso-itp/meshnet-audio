#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/spi_master.h"

#include "config/pins.h"
#include "config/build.h"
#include "control/display.h"
#include "control/buttons.h"
#include "control/status.h"
#include "network/mesh_net.h"

#include "vs1053/vs1053.h"
#include "packet.h"
#include "rx_pipeline.h"

extern void rx_pipeline_on_packet(const audio_packet_t* packet);

static const char* TAG = "main";
static vs1053_t vs1053;

static rx_status_t status = {
    .rssi = -100,
    .latency_ms = 0,
    .hops = 1,
    .receiving_audio = false,
    .bandwidth_kbps = 0
};

static uint8_t s_rx_buffer[MAX_PACKET_SIZE];

static void udp_receive_task(void* arg) {
    size_t actual_len;
    
    ESP_LOGI(TAG, "UDP receive task started");
    
    while (1) {
        if (network_udp_recv(s_rx_buffer, sizeof(s_rx_buffer), &actual_len, 100) == ESP_OK) {
            if (actual_len >= sizeof(audio_packet_t)) {
                audio_packet_t* packet = (audio_packet_t*)s_rx_buffer;
                rx_pipeline_on_packet(packet);
                status.receiving_audio = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void stats_update_task(void* arg) {
    while (1) {
        uint32_t rx_packets, dropped, underruns;
        rx_pipeline_get_stats(&rx_packets, &dropped, &underruns);
        
        status.receiving_audio = (rx_packets > 0);
        status.rssi = network_get_rssi();
        status.latency_ms = network_get_latency_ms();
        
        if (rx_packets > 0) {
            status.bandwidth_kbps = (PACKET_PAYLOAD_SIZE * 8 * 1000) / 
                                    (ADPCM_BLOCK_DURATION_MS * BLOCKS_PER_PACKET);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    esp_err_t ret;
    display_view_t current_view = DISPLAY_VIEW_NETWORK;
    
    ESP_LOGI(TAG, "Meshnet Audio VS1053 - RX (Receiver)");

#ifdef BOARD_ESP32S3
    ESP_LOGI(TAG, "Board: ESP32-S3");
#elif defined(BOARD_ESP32C6)
    ESP_LOGI(TAG, "Board: ESP32-C6");
#endif
    
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_ERROR_CHECK(display_init());
    ESP_ERROR_CHECK(buttons_init());
    
    ESP_LOGI(TAG, "Initializing VS1053...");
    ret = vs1053_init(&vs1053, VS1053_SPI_HOST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VS1053 initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check hardware connections:");
        ESP_LOGE(TAG, "  - SPI wiring (SCK, MISO, MOSI, CS)");
        ESP_LOGE(TAG, "  - DREQ and RESET pins");
        ESP_LOGE(TAG, "  - Power supply (3.3V)");
        ESP_LOGE(TAG, "Continuing with display-only mode...");
        
        while (1) {
            button_event_t btn_event = buttons_poll();
            if (btn_event == BUTTON_EVENT_SHORT_PRESS) {
                current_view = (current_view == DISPLAY_VIEW_NETWORK) ?
                              DISPLAY_VIEW_AUDIO : DISPLAY_VIEW_NETWORK;
            }
            display_render_rx(current_view, &status);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    ESP_LOGI(TAG, "VS1053 initialized successfully");
    ESP_ERROR_CHECK(vs1053_set_volume(&vs1053, 40, 40));
    
    ESP_LOGI(TAG, "Connecting to WiFi AP...");
    ret = network_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network STA initialization failed - cannot connect to TX");
        ESP_LOGE(TAG, "Make sure TX unit is powered on and in range");
        return;
    }
    ESP_LOGI(TAG, "WiFi connected to: %s", MESH_SSID);
    
    ESP_ERROR_CHECK(network_start_latency_measurement());
    
    ESP_LOGI(TAG, "Initializing RX audio pipeline...");
    ret = rx_pipeline_init(&vs1053, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RX pipeline init failed");
        return;
    }
    
    ret = rx_pipeline_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RX pipeline start failed");
        return;
    }
    
    ESP_LOGI(TAG, "RX pipeline started - decoding ADPCM to line-out");
    xTaskCreate(udp_receive_task, "udp_recv", 4096, NULL, 5, NULL);
    xTaskCreate(stats_update_task, "stats_update", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "RX system initialized - ready to receive audio");
    
    while (1) {
        button_event_t btn_event = buttons_poll();
        if (btn_event == BUTTON_EVENT_SHORT_PRESS) {
            current_view = (current_view == DISPLAY_VIEW_NETWORK) ?
                          DISPLAY_VIEW_AUDIO : DISPLAY_VIEW_NETWORK;
            ESP_LOGI(TAG, "View changed to %s",
                    current_view == DISPLAY_VIEW_NETWORK ? "Network" : "Audio");
        }
        
        display_render_rx(current_view, &status);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
