#pragma once

// Audio codec configuration (VS1053 ADPCM)
#define AUDIO_SAMPLE_RATE      48000
#define AUDIO_CHANNELS         1      // Mono (VS1053 LINE2 recording)
#define AUDIO_BITS_PER_SAMPLE  16     // VS1053 ADPCM internal resolution

// ADPCM block configuration
#define ADPCM_SAMPLES_PER_BLOCK  256
#define ADPCM_BLOCK_SIZE_BYTES   132  // Mono block (132 bytes for 256 samples)
#define ADPCM_BLOCK_DURATION_MS  (ADPCM_SAMPLES_PER_BLOCK * 1000 / AUDIO_SAMPLE_RATE)  // ~5.33ms

// Network packet configuration
#define BLOCKS_PER_PACKET      2
#define PACKET_HEADER_SIZE     12    // seq(4) + timestamp(4) + flags(2) + reserved(2)
#define PACKET_PAYLOAD_SIZE    (BLOCKS_PER_PACKET * ADPCM_BLOCK_SIZE_BYTES)
#define MAX_PACKET_SIZE        (PACKET_HEADER_SIZE + PACKET_PAYLOAD_SIZE)

// Network configuration
#define MESH_SSID              "MeshNet-Audio"
#define MESH_PASSWORD          "meshnet123"
#define UDP_PORT               3333

// Buffer configuration
#define TX_RING_BUFFER_BLOCKS  32    // ~170ms buffer
#define RX_JITTER_BUFFER_PKTS  8     // ~85ms jitter buffer
#define RX_PLAYOUT_TARGET_PKTS 4     // Start playback when 4 packets buffered

// Thresholds
#define TX_RING_LOW_WATERMARK  4
#define TX_RING_HIGH_WATERMARK 24
