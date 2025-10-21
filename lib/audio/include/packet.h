#pragma once

#include <stdint.h>
#include "config/build.h"

typedef struct __attribute__((packed)) {
    uint32_t seq;
    uint32_t timestamp_us;
    uint16_t flags;
    uint16_t reserved;
    uint8_t payload[PACKET_PAYLOAD_SIZE];
} audio_packet_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t codec;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t reserved1;
    uint16_t samples_per_block;
    uint16_t block_align;
    uint16_t reserved2;
} session_config_t;

#define SESSION_MAGIC     0x4D415344  // "MASD" = Meshnet Audio Stream Data
#define SESSION_VERSION   1
#define CODEC_ADPCM_IMA   1

#define PKT_FLAG_CONFIG   0x0001
