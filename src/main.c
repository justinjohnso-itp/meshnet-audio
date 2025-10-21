#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "config/pins.h"
#include "config/build.h"

#ifdef UNIT_TX
extern esp_err_t tx_pipeline_init(void* vs, void* udp);
extern esp_err_t tx_pipeline_start(void);
extern void tx_pipeline_get_stats(uint32_t* blocks_produced, uint32_t* packets_sent, uint32_t* buffer_fill);
#else
extern esp_err_t rx_pipeline_init(void* vs, void* udp);
extern esp_err_t rx_pipeline_start(void);
extern void rx_pipeline_get_stats(uint32_t* packets_received, uint32_t* packets_dropped, uint32_t* underruns);
#endif

// Placeholder VS1053 init
esp_err_t vs1053_init_placeholder(void) {
    return ESP_OK;
}

static const char* TAG = "main";

void app_main(void) {
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Meshnet Audio VS1053 Integration");
    
#ifdef UNIT_TX
    ESP_LOGI(TAG, "Unit: TX (Transmitter)");
#else
    ESP_LOGI(TAG, "Unit: RX (Receiver)");
#endif

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
    
    ESP_LOGI(TAG, "VS1053 driver ready (hardware integration pending)");
    ESP_LOGI(TAG, "Pin mappings configured for %s", 
#ifdef BOARD_ESP32S3
        "S3"
#else
        "C6"
#endif
    );
    
    ESP_LOGI(TAG, "System initialized successfully - ready for VS1053 hardware");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Running...");
    }
}
