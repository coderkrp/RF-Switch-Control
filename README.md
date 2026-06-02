# ⚡ RF-Switch-Control: Bare-Metal 433MHz Wireless Control System

[![MCU: CH32V003F4P6](https://img.shields.io/badge/MCU-CH32V003F4P6-blue)](https://www.wch.cn/products/CH32V003.html)
[![Arch: RISC-V RV32EC](https://img.shields.io/badge/Arch-RISC--V%20%28RV32EC%29-orange)](https://riscv.org/)
[![Language: C11](https://img.shields.io/badge/Language-C11-lightgrey)](https://en.wikipedia.org/wiki/C11_%28C_standard_library%29)
[![Framework: Bare-Metal](https://img.shields.io/badge/Framework-Bare--Metal%20%28noneos--sdk%29-brightgreen)](#)
[![Environment: PlatformIO](https://img.shields.io/badge/Environment-PlatformIO-blueviolet)](https://platformio.org/)
[![RF: 433.92MHz ASK/OOK](https://img.shields.io/badge/RF-433.92MHz%20ASK%2FOOK-red)](#)

A high-performance, ultra-low-power, bare-metal wireless control system implemented on the cost-efficient **CH32V003F4P6** (RISC-V) microcontroller. The project features a software-defined ASK/OOK physical layer protocol, custom frame decoding state machines, and deep standby power management (<10µA idle).

It uses a one-way RF link at 433.92 MHz with the standard **FS1000A** transmitter and **XY-MK-5V** receiver modules to remotely control four independent momentary switches with dedicated LED status outputs.

---

## 🏗️ System Architecture

The system is split into two independent nodes: the **Transmitter Unit** and the **Receiver Unit**.

```
┌──────────────────────────────────┐                    ┌─────────────────────────────────┐
│        TRANSMITTER UNIT          │                    │          RECEIVER UNIT          │
│       [CH32V003F4P6 MCU]         │                    │       [CH32V003F4P6 MCU]        │
├──────────────────────────────────┤                    ├─────────────────────────────────┤
│  PC4-PC7 ──▶ Input (Pull-Up)     │                    │  PC4-PC7 ──▶ Output (Push-Pull) │
│              [4x Push Buttons]   │                    │              [4x Status LEDs]   │
│                                  │                    │                                 │
│  PC1     ──▶ RF DATA OUT        │──── [433 MHz] ────▶│  PC1     ──▶ RF DATA IN         │
│              [FS1000A Modulator] │     OOK Link       │              [XY-MK-5V Receiver]│
│                                  │                    │                                 │
│  Power   ──▶ Standby WFE Mode   │                    │  Power   ──▶ Continuous Polling │
│              (<10µA Idle Current)│                    │              (SysTick & TIM2)   │
└──────────────────────────────────┘                    └─────────────────────────────────┘
```

### Key Engineering Features
* **Bare-Metal Firmware:** Written entirely in C using the WCH `noneos-sdk` framework, without any RTOS overhead, keeping flash and RAM footprint minimal.
* **Ultra-Low Power Standby (<10µA):** The transmitter MCU resides in deep standby mode (`WFE` mode) when idle. External GPIO interrupts on PD0-PD3 wake the processor instantly.
* **Software-Defined OOK Physical Layer:** Eliminates hardware serial overhead (no UART/SPI) by bit-banging precise pulse widths on the GPIO pins using microsecond-level hardware timer resolution.
* **Resilient State-Machine Receiver:** The receiver uses hardware Timer 2 (TIM2) in input capture style to decode the incoming raw bitstream, filter RF noise, validate addresses, verify XOR checksums, and timeout automatically.

---

## 📡 Custom RF Protocol Design

Since cheap 433MHz ASK modules (like XY-MK-5V) feature Automatic Gain Control (AGC) that amplifies background RF noise when idle, standard serial framing (UART) is highly prone to corruption. This project implements a custom software-defined protocol designed specifically for noisy ASK/OOK mediums.

### 1. Physical Layer: Pulse-Width Modulation (PWM)
Instead of standard NRZ (Non-Return-to-Zero) encoding, bits are represented by unequal HIGH-to-LOW pulse ratios:
* **Logic 0:** 500 µs HIGH pulse followed by a 500 µs LOW pulse (total 1.0 ms).
* **Logic 1:** 1000 µs HIGH pulse followed by a 500 µs LOW pulse (total 1.5 ms).

```
Logic 0:  ┌─────┐
          │     │
          └─────┴─────┘
          ◄─500─►◄─500─► (µs)

Logic 1:  ┌──────────┐
          │          │
          └──────────┴─────┘
          ◄──1000────►◄─500─► (µs)
```

*This variable-length symbol mapping ensures the receiver's AGC remains locked and makes the protocol self-clocking.*

### 2. Link Layer: Packet Framing
Data is sent in structured packets of **4 bytes (32 bits)**:

| Byte | Field Name | Value | Purpose |
| :---: | :--- | :---: | :--- |
| **0** | **Training Byte** | `0x55` | Sent before the packet to settle the receiver's Automatic Gain Control (AGC). |
| **1** | **Preamble / Sync** | `0xAA` | Used by the receiver to detect start of frame (SOF). |
| **2** | **Device Address** | `0x42` | 8-bit addressing to reject interference from nearby transmitters. |
| **3** | **Button State** | `0x0X` | 4-bit momentary bitmap (Bits 0-3 mapping to buttons 1-4). |
| **4** | **XOR Checksum** | `0xXX` | Calculated as `Preamble ^ Address ^ Button State` to verify integrity. |

---

## ⚡ Low-Power & Reliability Engineering

### 1. Transmitter Power Management
To maximize battery life on the transmitter, the system employs several advanced power-saving techniques:
* **Standby Mode via WFE:** When no buttons are pressed, the transmitter configures GPIO Port D Pins 0-3 as external event sources (`EXTI_Mode_Event`) on the falling edge (active low buttons). It then invokes `PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE)`.
* **Zero-Power ASK Idle State:** The FS1000A transmitter is connected directly to PC1. Since ASK (Amplitude Shift Keying) transmits power only when the input line is HIGH, holding PC1 `RESET` (LOW) disables the transmitter's internal oscillator, reducing transmitter power consumption to **0 mA** without requiring a dedicated power-gating MOSFET.
* **Unused GPIO Stabilization:** All unused GPIO pins are configured with internal pull-ups (`GPIO_Mode_IPU`) to prevent floating gate leakages that drain power.
* **Instant Wake & Debug Protection:** Standby wakeup acts as a system reset on the CH32V003. To permit in-circuit debugger connections, a standard 2-second startup safety delay is implemented. However, on wake from standby, the MCU detects the Low-Power Reset flag (`RCC_FLAG_LPWRRST`), clears it, and bypasses the safety delay—allowing instant button-press response (<15ms latency).

### 2. Receiver Demodulation State Machine
The receiver runs a robust non-blocking decoding loop:
1. **Edge Detection:** Reads PC1 and measures timing between edges using `TIM2` (1 µs tick rate).
2. **Pulsed Bit Demodulation:** 
   - A falling edge triggers high-pulse duration verification.
   - A rising edge triggers low-pulse verification (must be 300 µs – 700 µs).
   - If low-pulse timing is valid, the high pulse is classified into `0` or `1` based on timing thresholds.
3. **Preamble Synchronization:** If in `STATE_SEARCH_PREAMBLE`, incoming bits are shifted into a register. When the lower 8 bits match `0xAA`, the state transitions to `STATE_RECEIVE_DATA`.
4. **Validation:** After 32 bits are received, the system validates the address and the XOR checksum. If valid, LED states on `PD0-PD3` are immediately updated.
5. **Bit Timeout:** If the gap between bits exceeds **2.5 ms**, the state machine resets. This prevents partial transmissions or noise spikes from causing stuck frames.
6. **Safety Momentary Timeout:** If no valid packets are received for **100 ms**, a timeout function automatically clears all LED outputs. This prevents the outputs from being stuck "ON" if the transmitter button release packet is lost in transit due to RF collision.

### 3. Independent Watchdog Timer (IWDG) Integration
To ensure system-wide fault tolerance and defense against hardware lockups (such as unexpected EMI interference, power line glitches, or clock failures), both nodes run the hardware Independent Watchdog (IWDG):
* **Timeout Period:** Both nodes configure the IWDG with a prescaler of 32 and reload value of 1250. Ticking from the chip's internal Low-Speed Oscillator (LSI ~40kHz), this establishes a ~1.0-second recovery timeout window.
* **Transmitter Node:** The watchdog is initialized immediately after the 2-second debugger safety delay. It is fed at the start of each button scanning iteration. Before transitioning into deep Standby sleep (WFE), a final watchdog feed is performed. In deep Standby, the watchdog is automatically frozen by the chip's hardware configuration, preventing loop reset cycles.
* **Receiver Node:** The watchdog is initialized during boot after standard timer/GPIO setups and is fed continuously at the start of the `while (1)` event decoding loop. Any unexpected lockup in bit processing triggers a hardware reset within 1 second.

---

## 🔌 Pin Mapping

### Transmitter Pins (CH32V003F4P6)
| MCU Pin | Function | Direction | Configuration | Hardware Connection |
| :---: | :---: | :---: | :---: | :--- |
| **PC4** | Button 1 | Input | Internal Pull-Up | Tactile Switch (Active Low) |
| **PC5** | Button 2 | Input | Internal Pull-Up | Tactile Switch (Active Low) |
| **PC6** | Button 3 | Input | Internal Pull-Up | Tactile Switch (Active Low) |
| **PC7** | Button 4 | Input | Internal Pull-Up | Tactile Switch (Active Low) |
| **PC1** | RF DATA | Output | Push-Pull | FS1000A DATA IN |
| **PD1** | SWIO | I/O | SWIO Debug | Dedicated WCH-LinkE Debug/Flash |

### Receiver Pins (CH32V003F4P6)
| MCU Pin | Function | Direction | Configuration | Hardware Connection |
| :---: | :---: | :---: | :---: | :--- |
| **PC4** | LED 1 | Output | Push-Pull | Red LED (via Current Limiter) |
| **PC5** | LED 2 | Output | Push-Pull | Green LED (via Current Limiter) |
| **PC6** | LED 3 | Output | Push-Pull | Blue LED (via Current Limiter) |
| **PC7** | LED 4 | Output | Push-Pull | Yellow LED (via Current Limiter) |
| **PC1** | RF DATA | Input | Floating | XY-MK-5V DATA OUT |
| **PD1** | SWIO | I/O | SWIO Debug | Dedicated WCH-LinkE Debug/Flash |

---

## 🛠️ Project Structure

```
.
├── platformio.ini         # PlatformIO Multi-Environment Configuration
├── src/
│   ├── common/
│   │   └── protocol.h     # Shared RF parameters, sync word, and bit thresholds
│   ├── transmitter/
│   │   └── main.c         # Transmitter entry point, low power manager, & modulator
│   └── receiver/
│       └── main.c         # Receiver entry point, SysTick timer, & TIM2 demodulator
└── docs/
    ├── PRD.md           # Product Requirements Document
    ├── ARCHITECTURE.md  # Detailed System Architecture
    └── HARDWARE_GUIDE.md          # Hardware details, BOM, antenna & wiring guide
```

---

## 🚀 Building & Flashing

This project uses **PlatformIO** with the open-source community platform for CH32V MCUs.

### Prerequisites
1. Install [PlatformIO IDE](https://platformio.org/) (VS Code extension) or the PlatformIO CLI.
2. A WCH-LinkE programmer (connected to the SWIO line of the CH32V003).

### Using the Command Line

To build both environments:
```bash
pio run
```

To flash the transmitter node:
```bash
pio run -e transmitter --target upload
```

To flash the receiver node:
```bash
pio run -e receiver --target upload
```

---

## ⚙️ Key Engineering Principles
This repository implements several production-grade firmware design methodologies:
* **Custom Protocol Implementation:** Implementing robust physical and link layers from scratch to solve real-world hardware limits (ASK noise).
* **Deep System-Level Integration:** Configuring interrupts, timers, low-power registers, and clock trees at the register level using vendor headers.
* **Low-Power Optimization:** Achieving sub-microamp consumption of peripherals (like RF transceivers) through clever driver and pin-state management.
* **Defensive Firmware Design:** Incorporating recovery delays, timeout watchdogs, packet validation, and automatic output shutdowns to prevent lockups.
