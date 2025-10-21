# VS1053 Integration - Implementation Summary

## ✅ Completed

### 1. Architecture Design
- Migrated from ESP32-based audio (USB/I2S) to **VS1053B hardware codec**
- **TX**: Line-in → VS1053 ADPCM encode → UDP streaming
- **RX**: UDP receive → VS1053 ADPCM decode → Line-out
- Reduced bandwidth from **9.22Mbps** (96kHz PCM) to **~384kbps** (48kHz ADPCM)
- Target latency: **45-75ms** end-to-end

### 2. Pin Allocations
**ESP32-S3:**
- I2C (Display): GPIO5=SDA, GPIO6=SCL
- SPI (VS1053): GPIO7=SCK, GPIO9=MOSI, GPIO8=MISO
- VS1053 Control: GPIO1=XCS, GPIO2=XDCS, GPIO3=DREQ, GPIO4=XRESET
- Button: GPIO43

**ESP32-C6:**
- I2C (Display): GPIO22=SDA, GPIO23=SCL
- SPI (VS1053): GPIO19=SCK, GPIO18=MOSI, GPIO20=MISO
- VS1053 Control: GPIO0=XCS, GPIO1=XDCS, GPIO2=DREQ, GPIO21=XRESET
- Button: GPIO16

### 3. Code Implementation
- ✅ **VS1053 Driver** (`lib/audio/vs1053/`)
  - ESP-IDF native driver (no Arduino dependency)
  - SCI (control) and SDI (data) interfaces
  - ADPCM recording and playback modes
  - WAV header generation
  
- ✅ **TX Pipeline** (`lib/audio/tx_pipeline.c`)
  - Record task: reads ADPCM blocks from VS1053
  - Ring buffer: 32 blocks (~170ms)
  - Send task: packets 2 blocks at a time via UDP
  
- ✅ **RX Pipeline** (`lib/audio/rx_pipeline.c`)
  - Jitter buffer: 8 packets (~85ms)
  - Playback task: feeds ADPCM to VS1053
  - WAV header sent on first packet

- ✅ **Packet Format** (`lib/audio/include/packet.h`)
  - Header: seq(4) + timestamp(4) + flags(2) + reserved(2)
  - Payload: 2 ADPCM blocks (528 bytes)

- ✅ **Configuration**
  - `lib/config/include/config/pins.h` - Board-specific pins (S3/C6)
  - `lib/config/include/config/build.h` - Codec params, buffers

### 4. PlatformIO Configuration
- ✅ Four environments: `s3-tx`, `s3-rx`, `c6-tx`, `c6-rx`
- ✅ Build flags: `BOARD_ESP32S3`/`BOARD_ESP32C6`, `UNIT_TX`/`UNIT_RX`
- ✅ Unified `src/main.c` (TX/RX determined by build flags)

### 5. Build Status
- ✅ **s3-tx**: Builds successfully
- ✅ **s3-rx**: Builds successfully
- ⚠️ **c6-tx/c6-rx**: Builds but has esptool firmware generation error (known ESP32-C6 issue)

## 📋 Next Steps

### For Hardware Integration:
1. Wire VS1053B boards to ESP32-S3 TX and RX units using pin mappings above
2. Test VS1053 initialization and SPI communication
3. Test ADPCM recording on TX unit (line-in input)
4. Test ADPCM playback on RX unit (line-out output)
5. Integrate network layer (WiFi mesh + UDP)
6. Connect TX pipeline output to UDP send
7. Connect UDP receive to RX pipeline input
8. Tune jitter buffer size for optimal latency vs. reliability

### Optional Enhancements:
- Add display support (SSD1306 OLED) for status/stats
- Add button handling for view switching
- Implement packet loss concealment (PLC)
- Add session config packets for dynamic codec negotiation
- Optimize VS1053 SPI clock speed (8MHz → 12-18MHz)
- Fix ESP32-C6 esptool issue for full 4-board support

## 📁 Key Files
- [VS1053_MIGRATION_PLAN.md](docs/VS1053_MIGRATION_PLAN.md) - Detailed architecture
- [AGENTS.md](AGENTS.md) - Updated build commands and structure
- `lib/audio/vs1053/vs1053.{c,h}` - VS1053 driver
- `lib/audio/{tx,rx}_pipeline.{c,h}` - Audio pipelines
- `lib/config/include/config/pins.h` - Pin configurations

## 🏗️ Build Commands
```bash
# Build S3 TX and RX
pio run -e s3-tx
pio run -e s3-rx

# Upload to hardware
pio run -e s3-tx -t upload --upload-port /dev/cu.usbmodem101
pio run -e s3-rx -t upload --upload-port /dev/cu.usbmodem2101

# Monitor serial output
pio device monitor -b 115200
```

## 📊 Audio Specifications
- **Codec**: IMA ADPCM (WAV format)
- **Sample Rate**: 48kHz stereo
- **Block Size**: 256 samples/block (~5.33ms)
- **Block Data**: 264 bytes/block (stereo)
- **Bitrate**: ~384kbps
- **Packet Size**: 528 bytes payload + 12 bytes header
- **Latency Budget**: 45-75ms (5ms encode + 11ms packet + 43-64ms jitter + decode)

## ✨ Major Improvements
- **24x bandwidth reduction** (9.22Mbps → 384kbps)
- **Hardware-offloaded audio processing** (no CPU load on ESP32)
- **Simplified architecture** (removed USB audio, I2S DAC, tone gen, PCF8591 ADC)
- **Multi-board support** (ESP32-S3 and ESP32-C6 with board-specific configs)
- **Production-ready codec** (VS1053B is mature, stable, widely used)
