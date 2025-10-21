# VS1053 Meshnet Audio - Wiring Guide

## Components Required (Per Unit)

### TX Unit:
- 1× XIAO ESP32-S3
- 1× Adafruit VS1053B Codec Breakout
- 1× SSD1306 OLED Display (128×32, I2C, 0x3C)
- 1× Tactile button
- 1× 10kΩ pull-up resistor (for button)
- Audio source with 3.5mm line-out (phone, laptop, etc.)
- Breadboard and jumper wires

### RX Unit:
- 1× XIAO ESP32-S3
- 1× Adafruit VS1053B Codec Breakout
- 1× SSD1306 OLED Display (128×32, I2C, 0x3C)
- 1× Tactile button
- 1× 10kΩ pull-up resistor (for button)
- Headphones or powered speakers with 3.5mm input
- Breadboard and jumper wires

---

## S3-TX Wiring Table

| XIAO ESP32-S3 | Connection | VS1053B / Component |
|---------------|------------|---------------------|
| **Power** |
| 3V3 | → | VS1053 VCC |
| 3V3 | → | OLED VCC |
| GND | → | VS1053 GND |
| GND | → | OLED GND |
| GND | → | Button (one side) |
| **VS1053 SPI** |
| GPIO7 (SCK) | → | VS1053 SCK |
| GPIO9 (MOSI) | → | VS1053 MOSI |
| GPIO8 (MISO) | ← | VS1053 MISO |
| **VS1053 Control** |
| GPIO1 | → | VS1053 XCS (Chip Select) |
| GPIO2 | → | VS1053 XDCS (Data CS) |
| GPIO3 | ← | VS1053 DREQ (Data Request) |
| GPIO4 | → | VS1053 XRESET (Reset) |
| **I2C Display** |
| GPIO5 | ↔ | OLED SDA |
| GPIO6 | → | OLED SCL |
| **Button** |
| GPIO43 | → | Button (other side via 10kΩ pull-up to 3V3) |
| **Audio Input** |
| (External) | → | VS1053 LINE-IN Left |
| (External) | → | VS1053 LINE-IN Right |
| (External) | → | VS1053 LINE-IN Ground |

### Notes for TX:
- Connect your audio source (phone, laptop) to the VS1053 LINE-IN jack
- Adjust volume on the source device (VS1053 has fixed gain)
- Button should have 10kΩ pull-up resistor to 3V3

---

## S3-RX Wiring Table

| XIAO ESP32-S3 | Connection | VS1053B / Component |
|---------------|------------|---------------------|
| **Power** |
| 3V3 | → | VS1053 VCC |
| 3V3 | → | OLED VCC |
| GND | → | VS1053 GND |
| GND | → | OLED GND |
| GND | → | Button (one side) |
| **VS1053 SPI** |
| GPIO7 (SCK) | → | VS1053 SCK |
| GPIO9 (MOSI) | → | VS1053 MOSI |
| GPIO8 (MISO) | ← | VS1053 MISO |
| **VS1053 Control** |
| GPIO1 | → | VS1053 XCS (Chip Select) |
| GPIO2 | → | VS1053 XDCS (Data CS) |
| GPIO3 | ← | VS1053 DREQ (Data Request) |
| GPIO4 | → | VS1053 XRESET (Reset) |
| **I2C Display** |
| GPIO5 | ↔ | OLED SDA |
| GPIO6 | → | OLED SCL |
| **Button** |
| GPIO43 | → | Button (other side via 10kΩ pull-up to 3V3) |
| **Audio Output** |
| (External) | ← | VS1053 LINE-OUT Left |
| (External) | ← | VS1053 LINE-OUT Right |
| (External) | ← | VS1053 LINE-OUT Ground |

### Notes for RX:
- Connect headphones or powered speakers to the VS1053 LINE-OUT jack
- If using passive speakers, you may need an amplifier
- Volume control can be adjusted via VS1053 software registers

---

## Important Wiring Notes

### Power:
- **Always use 3.3V** for both VS1053 and OLED
- Do NOT connect 5V to VS1053 or OLED (they are 3.3V devices)
- Ensure all GND connections are common

### SPI Connections:
- Keep wires **short** (< 15cm ideally) to minimize signal integrity issues
- Consider 10-33Ω series resistors on SCK/MOSI if you see signal ringing
- MISO is input to ESP32, MOSI/SCK are outputs

### VS1053 Specific:
- **DREQ** is an input to ESP32 - it signals when VS1053 is ready for data
- **XCS** = Command Chip Select (for SCI register access)
- **XDCS** = Data Chip Select (for SDI audio data stream)
- **XRESET** should be pulled HIGH after power-up

### OLED Display:
- I2C address should be **0x3C** (default for most SSD1306 modules)
- If your OLED has a different address, update `DISPLAY_I2C_ADDR` in `config/pins.h`
- SDA/SCL have internal pull-ups on most OLED modules

### Button:
- One side to GPIO43, other side to GND
- 10kΩ pull-up resistor from GPIO43 to 3V3
- Button press pulls GPIO43 LOW

---

## Breadboard Layout Suggestions

### TX Unit Layout:
```
[ESP32-S3]     [VS1053B]        [OLED]
   |              |               |
   |-- SPI Bus ---|               |
   |                              |
   |-------- I2C Bus -------------|
   |
[Button]  [Audio Source]
```

### RX Unit Layout:
```
[ESP32-S3]     [VS1053B]        [OLED]
   |              |               |
   |-- SPI Bus ---|               |
   |                              |
   |-------- I2C Bus -------------|
   |
[Button]  [Headphones/Speakers]
```

---

## Testing Procedure

### 1. Power Check:
- Before connecting ESP32, verify VS1053 has 3.3V on VCC
- Check OLED has 3.3V on VCC
- Verify all GNDs are connected

### 2. Upload Firmware:
```bash
# TX Unit
pio run -e s3-tx -t upload --upload-port /dev/cu.usbmodem101

# RX Unit
pio run -e s3-rx -t upload --upload-port /dev/cu.usbmodem2101
```

### 3. Monitor Serial Output:
```bash
pio device monitor -b 115200
```

Look for:
- "VS1053 initialized successfully"
- "VS1053 driver ready (hardware integration pending)"
- No SPI errors

### 4. Hardware Verification:
- Measure voltage on VS1053 VCC pin (should be 3.3V)
- Check DREQ pin (should toggle when VS1053 is active)
- Verify OLED lights up (if display code is enabled)

---

## Troubleshooting

### VS1053 Not Initializing:
- Check SPI connections (SCK, MOSI, MISO)
- Verify XCS, XDCS are connected
- Ensure XRESET is HIGH
- Check power (3.3V on VCC)

### No Audio on TX:
- Verify LINE-IN jack is fully inserted
- Check source volume is not muted
- Confirm VS1053 DREQ is toggling

### No Audio on RX:
- Check LINE-OUT connections
- Verify headphones/speakers are powered (if needed)
- Confirm VS1053 is decoding (DREQ should toggle)

### OLED Not Working:
- Verify I2C address (0x3C default)
- Check SDA/SCL connections
- Ensure pull-up resistors are present (usually on module)

### Button Not Responding:
- Verify pull-up resistor (10kΩ to 3V3)
- Check button is connected to GPIO43
- Test continuity with multimeter
