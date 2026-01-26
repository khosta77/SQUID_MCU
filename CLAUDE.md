# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Firmware for STM32F407VG microcontroller controlling up to 10 stepper motors in synchronous and asynchronous modes. Uses pure CMSIS (no HAL/LL libraries).

**Target:** STM32F407VG (ARM Cortex-M4, 168 MHz, 1MB Flash, 192KB SRAM)

## Build Commands

```bash
# Full build (generates main.elf, main.hex, main.bin)
make all

# Clean build artifacts
make clean
```

**Toolchain:** arm-none-eabi-gcc, arm-none-eabi-g++

## Architecture

### Hardware Interface

```
PC (Client) <--USB--> FTDI FT232RL <--UART4--> STM32F407VG <--USART2--> Motor Drivers (1-10)
```

### GPIO Pinout
- **PA0-PA1:** UART4 (PC communication)
- **PA2-PA3:** USART2 (motor drivers)
- **PB0-PB9:** KEY1-10 (data transmission enable)
- **PC0-PC9:** EN1-10 (driver power enable)
- **PD0-PD9:** SELECT1-10 (driver selection)
- **PE0-PE9:** STATUS1-10 (motor status interrupts)
- **PE10-PE15:** ENDSTOP1-6 (emergency stop)

### DMA Channels
- **DMA1_Stream2:** UART4_RX (commands from PC)
- **DMA1_Stream6:** USART2_TX (data to drivers)

### Code Structure

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point, DMA/interrupt handlers, system init |
| `motor_controller.cpp/hpp` | Motor control logic, command processing |
| `motor_settings.cpp/hpp` | MotorSettings class (16 bytes per motor) |
| `constants.cpp/hpp` | Protocol constants, global state variables |
| `serial.cpp/hpp` | UART/USART initialization |
| `gpio.cpp/hpp` | GPIO initialization |

### Protocol

Commands (1 byte):
- `0x8N` - Synchronous mode (N = motor count 1-10)
- `0x4N` - Asynchronous mode (N = motor count 1-10)
- `0x20` - Firmware version request

Motor data: 16 bytes per motor (number, acceleration, maxSpeed, steps - all uint32_t)

Response codes:
- `0x00` - Ready
- `0xFF` - Success
- `0x0B` - Emergency stop

## Key Implementation Details

- All peripheral access via direct CMSIS register manipulation
- Interrupts: EXTI0-9 for motor status, EXTI10-15 for emergency stop (highest priority)
- Global state flags (`volatile`) track motor completion and emergency conditions
- DMA used for both command reception and driver communication
