#pragma once

#include "vs1053/vs1053.h"
#include "esp_err.h"

typedef struct udp_handle* udp_handle_t;

esp_err_t rx_pipeline_init(vs1053_t* vs, udp_handle_t udp);
esp_err_t rx_pipeline_start(void);
esp_err_t rx_pipeline_stop(void);

void rx_pipeline_get_stats(uint32_t* packets_received, uint32_t* packets_dropped, uint32_t* underruns);
