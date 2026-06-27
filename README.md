# ESP32-S3 ST7789 LVGL Controller

Desktop volume controller prototype for PC based on `ESP32-S3`, `ST7789`, `LVGL v9`, and `LovyanGFX`.

## Stack

- `PlatformIO`
- `Arduino` framework
- `ESP32-S3`
- `LVGL v9`
- `LovyanGFX`
- `mathertel/RotaryEncoder@^1.6.0`

## Hardware

- Display: `ST7789V3` in landscape mode `280x240`
- MCU: `ESP32-S3`
- Encoder 1: main UI navigation and actions
- Encoder 2: connected in hardware, logic not assigned yet

## Current State

This project is currently in the local demo stage:

- UI runs fully on the microcontroller
- Data is mocked locally in firmware
- No PC communication protocol is implemented yet
- Active work is done in branch `dev-work`

Implemented screens include:

- Master volume
- App volume
- Output devices
- Input devices

Removed from the current firmware branch:

- Playback screen
- System info screen

## Project Structure

- `src/main.cpp` — application logic and UI
- `include/my_display.hpp` — `LovyanGFX` display configuration
- `include/lv_conf.h` — `LVGL` configuration
- `platformio.ini` — PlatformIO environment and dependencies

## Build

Open the project in `VS Code` with `PlatformIO` and build the default environment from `platformio.ini`.

## Notes

- Native USB CDC is enabled through build flags
- Display rotation and offsets are tuned for the current hardware
- Encoder rotation logic is now handled by `RotaryEncoder` instead of manual phase decoding
- This repository currently contains firmware only

## Planned Next Phases

1. PC-side data collector script
2. Serial protocol between PC and ESP32-S3
3. Full integration and bug fixing




