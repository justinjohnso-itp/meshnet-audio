#pragma once

#include <stdint.h>

// VS1053 SCI (Serial Command Interface) Register Addresses
#define VS1053_SCI_MODE        0x00
#define VS1053_SCI_STATUS      0x01
#define VS1053_SCI_BASS        0x02
#define VS1053_SCI_CLOCKF      0x03
#define VS1053_SCI_DECODE_TIME 0x04
#define VS1053_SCI_AUDATA      0x05
#define VS1053_SCI_WRAM        0x06
#define VS1053_SCI_WRAMADDR    0x07
#define VS1053_SCI_HDAT0       0x08
#define VS1053_SCI_HDAT1       0x09
#define VS1053_SCI_AIADDR      0x0A
#define VS1053_SCI_VOL         0x0B
#define VS1053_SCI_AICTRL0     0x0C
#define VS1053_SCI_AICTRL1     0x0D
#define VS1053_SCI_AICTRL2     0x0E
#define VS1053_SCI_AICTRL3     0x0F

// SCI_MODE bits
#define SM_DIFF          0x0001
#define SM_LAYER12       0x0002
#define SM_RESET         0x0004
#define SM_CANCEL        0x0008
#define SM_EARSPEAKER_LO 0x0010
#define SM_TESTS         0x0020
#define SM_STREAM        0x0040
#define SM_EARSPEAKER_HI 0x0080
#define SM_DACT          0x0100
#define SM_SDIORD        0x0200
#define SM_SDISHARE      0x0400
#define SM_SDINEW        0x0800
#define SM_ADPCM         0x1000
#define SM_LINE1         0x4000
#define SM_CLK_RANGE     0x8000

// SCI_STATUS bits
#define SS_DO_NOT_JUMP   0x8000
#define SS_SWING_HIGH    0x1000
#define SS_SWING_LOW     0x0800
#define SS_AVOL_DOWN     0x0400
#define SS_AVOL_UP       0x0200
#define SS_AD_CLOCK      0x0002
#define SS_REFERENCE_SEL 0x0001

// SCI_CLOCKF default value (multiply by 3.5x)
#define VS1053_CLOCKF_DEFAULT 0x8800  // SC_MULT=3.5x, SC_ADD=0

// AICTRL register indices for ADPCM recording
#define AICTRL_PROFILE       0
#define AICTRL_SAMPLERATE    1
#define AICTRL_RESERVED      2
#define AICTRL_GAIN          3

// ADPCM profiles for SCI_AICTRL0
#define ADPCM_PROFILE_IMA    0  // IMA ADPCM encoding

// Volume: 0x0000 = max, 0xFEFE = mute
#define VS1053_VOL_MAX       0x0000
#define VS1053_VOL_MUTE      0xFEFE
#define VS1053_VOL_DEFAULT   0x2020  // Moderate volume

// WAV header constants
#define WAV_RIFF_CHUNK_ID    0x46464952  // "RIFF"
#define WAV_WAVE_FORMAT      0x45564157  // "WAVE"
#define WAV_FMT_CHUNK_ID     0x20746D66  // "fmt "
#define WAV_DATA_CHUNK_ID    0x61746164  // "data"
#define WAV_FORMAT_ADPCM     0x0011      // IMA ADPCM format code
