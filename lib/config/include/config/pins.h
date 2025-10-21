#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"

#ifdef BOARD_ESP32S3
    // ESP32-S3 Pin Configuration
    // I2C pins for SSD1306 display (board defaults)
    #define I2C_MASTER_SDA_IO      GPIO_NUM_5
    #define I2C_MASTER_SCL_IO      GPIO_NUM_6
    
    // SPI pins for VS1053 (board defaults)
    #define VS1053_SPI_SCK         GPIO_NUM_7
    #define VS1053_SPI_MISO        GPIO_NUM_8
    #define VS1053_SPI_MOSI        GPIO_NUM_9
    
    // VS1053 control pins
    #define VS1053_XCS             GPIO_NUM_1
    #define VS1053_XDCS            GPIO_NUM_2
    #define VS1053_DREQ            GPIO_NUM_3
    #define VS1053_XRESET          GPIO_NUM_4
    
    // Button
    #define BUTTON_GPIO            GPIO_NUM_43

#elif defined(BOARD_ESP32C6)
    // ESP32-C6 Pin Configuration
    // I2C pins for SSD1306 display
    #define I2C_MASTER_SDA_IO      GPIO_NUM_22
    #define I2C_MASTER_SCL_IO      GPIO_NUM_23
    
    // SPI pins for VS1053
    #define VS1053_SPI_SCK         GPIO_NUM_19
    #define VS1053_SPI_MISO        GPIO_NUM_20
    #define VS1053_SPI_MOSI        GPIO_NUM_18
    
    // VS1053 control pins
    #define VS1053_XCS             GPIO_NUM_0
    #define VS1053_XDCS            GPIO_NUM_1
    #define VS1053_DREQ            GPIO_NUM_2
    #define VS1053_XRESET          GPIO_NUM_21
    
    // Button
    #define BUTTON_GPIO            GPIO_NUM_16

#else
    #error "Board not defined. Use -D BOARD_ESP32S3 or -D BOARD_ESP32C6"
#endif

// Common I2C configuration
#define I2C_MASTER_FREQ_HZ     400000
#define I2C_MASTER_NUM         I2C_NUM_0

// SSD1306 OLED configuration
#define DISPLAY_I2C_ADDR       0x3C
#define DISPLAY_WIDTH          128
#define DISPLAY_HEIGHT         32

// VS1053 SPI configuration
#define VS1053_SPI_HOST        SPI2_HOST
#define VS1053_SPI_FREQ_HZ     (8 * 1000 * 1000)  // Start at 8MHz, can increase to 12-18MHz after tuning
