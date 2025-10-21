# VS1053 Adafruit Library Issue - Current Status

**Date:** Oct 21, 2025  
**Branch:** `vs1053-integration`  
**Status:** ⚠️ BLOCKED - Adafruit library doesn't support ADPCM recording

## Problem Summary

The Adafruit VS1053 Arduino library **does not support IMA ADPCM recording**. It only supports:
- Audio playback (MP3, AAC, WAV, etc.)
- **Ogg Vorbis recording** (requires SD card and plugin file)

Our architecture requires low-latency IMA ADPCM recording for real-time UDP streaming, which requires direct VS1053 register access that the Adafruit library doesn't expose.

## What We've Done

### 1. Created Adafruit Adapter (Oct 21)
- **File:** `lib/audio/src/vs1053_adafruit_adapter.cpp`
- Wrapped Adafruit C++ library calls into our existing C API
- Fixed CMake build system issues
- Got both TX and RX units to initialize and boot successfully

### 2. Fixed ADPCM Initialization Logic (Oct 21)
- Implemented proper VS1053 ADPCM recording mode activation per datasheet
- Key fixes:
  - Set `SM_ADPCM` **during** `SM_RESET` (required for record firmware activation)
  - Fixed `SCI_AICTRL1`: use sample rate in Hz (was incorrectly `/1000`)
  - Added `SCI_AICTRL2` configuration for AGC
  - Fixed `SCI_HDAT1` interpretation: reads **word count**, not byte count
- Made TX pipeline accept variable-size ADPCM blocks

### 3. Debugging Revealed the Core Issue
- TX unit boots successfully
- VS1053 initializes properly
- WiFi connects correctly
- **BUT:** `SCI_HDAT1` register always returns 0 (no recording data available)
- Reason: Adafruit library's `sciWrite()` and `sciRead()` methods don't properly handle ADPCM recording mode registers

## Current Code State

### TX Unit (s3-tx)
- ✅ Builds and uploads successfully
- ✅ VS1053 hardware initializes
- ✅ WiFi AP starts (`MeshNet-Audio`)
- ❌ ADPCM recording produces 0 bytes (`bytes_read=0`)
- ❌ TX pipeline has no data to send

### RX Unit (s3-rx)
- ✅ Builds and uploads successfully
- ✅ VS1053 hardware initializes
- ✅ WiFi connects to TX AP
- ⚠️ Not tested (waiting for TX to produce audio data)

## Technical Details

### VS1053 ADPCM Recording Requirements (from datasheet)
1. Write `SCI_MODE = SM_SDINEW | SM_RESET | SM_ADPCM` (0x1804)
2. Wait for DREQ pin to go high
3. Configure AICTRL registers:
   - `SCI_AICTRL0 = 0` (IMA ADPCM profile)
   - `SCI_AICTRL1 = sample_rate` (in Hz, e.g., 48000)
   - `SCI_AICTRL2 = 0` (default AGC)
   - `SCI_AICTRL3 = 0 or 1` (mono/stereo)
4. Read `SCI_HDAT1` to get available data word count
5. Read data words from `SCI_HDAT0`

### What the Adafruit Library Supports
```cpp
// Playback - ✅ Fully supported
void playData(uint8_t *buffer, uint8_t buffsiz);
boolean startPlayingFile(const char *trackname);

// Ogg Vorbis Recording - ✅ Supported (requires SD + plugin)
boolean prepareRecordOgg(char *plugin);
void startRecordOgg(boolean mic);
uint16_t recordedWordsWaiting(void);
uint16_t recordedReadWord(void);

// ADPCM Recording - ❌ NOT SUPPORTED
// No API for ADPCM mode, direct register access needed
```

## Next Steps (TODO)

### Option 1: Custom VS1053 Driver (RECOMMENDED)
**Effort:** 2-4 hours  
**Approach:** Write minimal SPI driver for ADPCM recording/playback

1. **Remove Adafruit dependency completely**
   - Delete `vs1053_adafruit_adapter.cpp`
   - Remove from `platformio.ini` dependencies

2. **Implement direct SPI functions** in `lib/audio/vs1053/vs1053.c`:
   - `vs1053_spi_init()` - Configure ESP32 SPI bus
   - `vs1053_sci_write_direct()` - Write to command registers
   - `vs1053_sci_read_direct()` - Read from command registers
   - `vs1053_sdi_write_direct()` - Write audio data
   - Proper DREQ polling between operations

3. **Key implementation notes:**
   - Use ESP-IDF SPI driver (`spi_master.h`)
   - CS (XCS) for command interface
   - DCS (XDCS) for data interface
   - Poll DREQ before each SPI transaction
   - 1MHz SPI clock for SCI, up to 8MHz for SDI

4. **Reference implementations:**
   - Original custom driver (pre-Adafruit migration)
   - VS1053 datasheet sections 7-10
   - VLSI application notes

### Option 2: Switch to Ogg Vorbis (NOT RECOMMENDED)
**Effort:** 6-8 hours  
**Issues:**
- Requires SD card hardware (not currently on board)
- Higher latency (not suitable for real-time streaming)
- Need to redesign packet structure and pipeline
- More complex buffering logic

## Files Modified in This Session

```
lib/audio/src/vs1053_adafruit_adapter.cpp   - Fixed ADPCM init logic
lib/audio/tx_pipeline.c                      - Accept variable block sizes
lib/audio/rx_pipeline.c                      - No changes (blocked on TX)
platformio.ini                               - Adafruit library added
lib/audio/CMakeLists.txt                     - Component registration
```

## Debugging Commands

### Build and Upload TX
```bash
pio run -e s3-tx
pio run -e s3-tx -t upload --upload-port /dev/cu.usbmodem101
```

### Build and Upload RX
```bash
pio run -e s3-rx
pio run -e s3-rx -t upload --upload-port /dev/cu.usbmodem2101
```

### Monitor Serial Output
```bash
stty -f /dev/cu.usbmodem101 115200 && cat /dev/cu.usbmodem101  # TX unit
stty -f /dev/cu.usbmodem2101 115200 && cat /dev/cu.usbmodem2101  # RX unit
```

### Current Debug Output
```
I (xxxxx) vs1053_adafruit: HDAT1 words_available=0
I (xxxxx) tx_pipeline: Record: bytes_read=0, blocks_produced=0
```

## Architecture Reminder

```
TX UNIT (s3-tx)
  Line-in → VS1053 ADPCM encode → Ring Buffer → UDP Send

RX UNIT (s3-rx)
  UDP Receive → Jitter Buffer → VS1053 ADPCM decode → Line-out
```

## Key Configuration
- **Sample Rate:** 48 kHz
- **Channels:** Mono (1 channel)
- **Codec:** IMA ADPCM
- **Block Size:** 256 samples = 132 bytes
- **Packet Size:** 2 blocks = 264 bytes payload
- **Network:** WiFi (TX=AP, RX=STA), UDP port 3333

## References
- VS1053 Datasheet: https://cdn-shop.adafruit.com/datasheets/vs1053.pdf
- Adafruit Tutorial: https://learn.adafruit.com/adafruit-vs1053-mp3-aac-ogg-midi-wav-play-and-record-codec-tutorial
- Adafruit Library: https://github.com/adafruit/Adafruit_VS1053_Library

## Contact/Notes for Morning Session
- Both units are functional at hardware/WiFi level
- Only blocker is ADPCM recording activation
- Custom driver is the right path forward
- Can reference original pre-Adafruit code if needed
