# ESP32-S3 ST7789 LVGL Controller

Desktop volume/media controller prototype for PC based on `ESP32-S3`, `ST7789`, `LVGL v9`, and `LovyanGFX`.

## Stack

- `PlatformIO`
- `Arduino` framework
- `ESP32-S3`
- `LVGL v9`
- `LovyanGFX`
- `RotaryEncoder`

## Hardware

- Display: `ST7789V3` `240x280`
- MCU: `ESP32-S3`
- Encoder 1: main UI navigation and actions
- Encoder 2: connected in hardware, logic not assigned yet

## Current State

This project is currently in the local demo stage:

- UI runs fully on the microcontroller
- Data is mocked locally in firmware
- No PC communication protocol is implemented yet

Implemented screens include:

- Master volume
- App volume
- Output devices
- Input devices
- Playback
- System info

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
- This repository currently contains firmware only

## Planned Next Phases

1. PC-side data collector script
2. Serial protocol between PC and ESP32-S3
3. Full integration and bug fixing




