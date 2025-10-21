# VS1053 Integration Migration Plan

## Overview
Migrating from ESP32-based audio processing (USB/I2S/Tone) to VS1053B hardware codec for both TX and RX units.

## Architecture Changes

### Before
- **TX**: USB audio / Tone gen / AUX (PCF8591 ADC) → UDP streaming (96kHz/24-bit PCM)
- **RX**: UDP receive → I2S DAC (UDA1334) → Speakers/headphones
- **Bandwidth**: ~9.22Mbps (unsustainable on WiFi mesh)

### After
- **TX**: Line-in (aux) → **VS1053 ADPCM encode** → UDP streaming (48kHz stereo ADPCM)
- **RX**: UDP receive → **VS1053 ADPCM decode** → Line-out (aux)
- **Bandwidth**: ~384kbps (24x reduction)
- **Latency**: ~45-75ms end-to-end

## Audio Format
- **Codec**: IMA ADPCM (WAV ADPCM)
- **Sample Rate**: 48kHz stereo
- **Block Size**: 256 samples/block = ~5.33ms
- **Block Data**: 264 bytes/block (stereo)
- **Packet Format**: 2 blocks/packet (~528 bytes payload + 12-byte header)
- **Jitter Buffer**: 4-6 packets (~43-64ms)

## Pin Allocations

### XIAO ESP32-S3 (s3-tx, s3-rx)
| Function | Pin | Notes |
|----------|-----|-------|
| I2C SDA | GPIO5 | SSD1306 OLED (board default) |
| I2C SCL | GPIO6 | SSD1306 OLED (board default) |
| SPI SCK | GPIO7 | VS1053 (board default) |
| SPI MOSI | GPIO9 | VS1053 (board default) |
| SPI MISO | GPIO8 | VS1053 (board default) |
| XCS (SCI CS) | GPIO1 | VS1053 control |
| XDCS (SDI CS) | GPIO2 | VS1053 data |
| DREQ | GPIO3 | VS1053 ready signal |
| XRESET | GPIO4 | VS1053 reset |
| Button | GPIO43 | View cycling |

### XIAO ESP32-C6 (c6-tx, c6-rx)
| Function | Pin | Notes |
|----------|-----|-------|
| I2C SDA | GPIO22 | SSD1306 OLED |
| I2C SCL | GPIO23 | SSD1306 OLED |
| SPI SCK | GPIO19 | VS1053 |
| SPI MOSI | GPIO18 | VS1053 |
| SPI MISO | GPIO20 | VS1053 |
| XCS (SCI CS) | GPIO0 | VS1053 control |
| XDCS (SDI CS) | GPIO1 | VS1053 data |
| DREQ | GPIO2 | VS1053 ready signal |
| XRESET | GPIO21 | VS1053 reset |
| Button | GPIO16 | View cycling |

## Data Flow

### TX Pipeline
1. VS1053 records line-in audio (ADPCM encoding)
2. ESP32 reads 256-sample blocks via SCI (when DREQ high)
3. Blocks pushed to ring buffer (32 blocks = ~170ms)
4. UDP task pops 2 blocks, builds packet with [seq, timestamp, payload]
5. Sends via UDP unicast/broadcast

### RX Pipeline
1. UDP task receives packets, validates, stores in jitter buffer (8 packets)
2. On first packet, generate WAV ADPCM header, send to VS1053 via SDI
3. Once jitter buffer reaches target fill (4-6 packets), start playback
4. VS1053 playback task dequeues packets, feeds blocks to SDI (when DREQ high)
5. VS1053 decodes and outputs to line-out

## Library Structure Changes

```
lib/
├── config/
│   ├── pins_s3.h          # S3-specific pins
│   ├── pins_c6.h          # C6-specific pins
│   ├── pins.h             # Auto-include based on build flag
│   └── build_constants.h  # Codec params, buffers, UDP port
├── network/
│   ├── udp_tx.c/h         # Enhanced with seq/timestamp
│   └── udp_rx.c/h         # Enhanced with jitter buffer
├── audio/
│   ├── vs1053/
│   │   ├── vs1053.c/h         # ESP-IDF driver (SCI/SDI/ADPCM)
│   │   └── vs1053_regs.h      # Register definitions
│   ├── tx_pipeline.c/h        # Record → UDP
│   ├── rx_pipeline.c/h        # UDP → Playback
│   └── packet.h               # AudioPkt, SessionConfig structs
└── control/
    └── (unchanged - display, button, status)
```

## Removed Components
- ❌ USB audio input (lib/audio/usb_*)
- ❌ I2S DAC output (lib/audio/i2s_*)
- ❌ Tone generator (lib/audio/tone_*)
- ❌ PCF8591 ADC (future aux input)
- ❌ Input mode switching (TX)
- ❌ Potentiometer control

## PlatformIO Environments
- `s3-tx`: ESP32-S3 TX unit
- `s3-rx`: ESP32-S3 RX unit
- `c6-tx`: ESP32-C6 TX unit
- `c6-rx`: ESP32-C6 RX unit

## Implementation Phases
1. ✅ Pin allocation & architecture design
2. ⏳ VS1053 driver implementation (SCI/SDI, ADPCM)
3. ⏳ TX pipeline (record → ring buffer → UDP)
4. ⏳ RX pipeline (jitter buffer → VS1053 playback)
5. ⏳ Control layer updates (display views)
6. ⏳ PlatformIO config for 4 environments
7. ⏳ Testing all 4 configs

## Build Commands (Updated)
- `pio run -e s3-tx`
- `pio run -e s3-rx`
- `pio run -e c6-tx`
- `pio run -e c6-rx`

## Testing Strategy
1. Test S3-TX + S3-RX pair first (hardware available)
2. Verify ADPCM encoding/decoding
3. Test network streaming with packet loss scenarios
4. Port to C6 and verify all 4 configs build
5. Cross-test (S3-TX + C6-RX, etc.)
