# 🏗️ ARCHITECTURE DOCUMENT - CORRECTED

## Wireless RF Control System Architecture

---

### 1. System Overview

#### 1.1 High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          RF CONTROL SYSTEM                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────┐                    ┌─────────────────────────┐ │
│  │    TRANSMITTER UNIT     │                    │    RECEIVER UNIT        │ │
│  │    (FS1000A Module)     │                    │    (XY-MK-5V Module)    │ │
│  ├─────────────────────────┤                    ├─────────────────────────┤ │
│  │                         │                    │                         │ │
│  │  ┌───────────────────┐  │                    │  ┌───────────────────┐  │ │
│  │  │   CH32V003F4P6    │  │                    │  │   CH32V003F4P6    │  │ │
│  │  │   Microcontroller │  │                    │  │   Microcontroller │  │ │
│  │  └─────────┬─────────┘  │                    │  └─────────┬─────────┘  │ │
│  │            │            │                    │            │            │ │
│  │  PD0-PD3   │            │                    │  PD0-PD3   │            │ │
│  │  │    │    │            │                    │  │    │    │            │ │
│  │  ▼    ▼    ▼            │                    │  ▼    ▼    ▼            │ │
│  │  ┌────┐┌────┐┌────┐     │                    │  ┌────┐┌────┐┌────┐     │ │
│  │  │LED0││LED1││LED2│     │                    │  │LED0││LED1││LED2│     │ │
│  │  │(R) ││(G) ││(B) │     │                    │  │(R) ││(G) ││(B) │     │ │
│  │  └────┘└────┘└────┘     │                    │  └────┘└────┘└────┘     │ │
│  │                         │                    │                         │ │
│  │  PC1 ──── RF ───────────▶│                    │◀──────── RF ───────── PC1│ │
│  │        ▲                │                    │        ▲                │ │
│  │        │                │                    │        │                │ │
│  │  PD0-PD3 ──── Buttons ─▶│                    │◀────── Buttons ───────── PD0-│ │
│  │                         │                    │                         │ │
│  └─────────────────────────┘                    └─────────────────────────┘ │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 1.2 Component Hierarchy

```
System (RF Control System)
├── Transmitter Unit
│   ├── CH32V003F4P6 MCU
│   │   ├── GPIO Subsystem
│   │   │   ├── Input: PD0-PD3 (Switches)
│   │   │   └── Output: PD0-PD3 (LEDs - disabled)
│   │   └── RF Subsystem
│   │       ├── FS1000A Module
│   │       │   └── Data Interface (3-wire: VCC, GND, DATA)
│   │       └── Power Management
│   │           └── External transistor/MOSFET for EN control
│   └── Power Supply
│       ├── 3.3V Regulator
│       └── Battery/USB Interface
├── Receiver Unit
│   ├── CH32V003F4P6 MCU
│   │   ├── GPIO Subsystem
│   │   │   ├── Input: PD0-PD3 (Buttons)
│   │   │   └── Output: PD0-PD3 (LEDs)
│   │   └── RF Subsystem
│   │       ├── XY-MK-5V Module
│   │       │   └── Signal Detection
│   │       └── Power Management
│   └── Power Supply
│       ├── 3.3V/5V Regulator
│       └── External Power Interface
└── Communication Layer
    ├── RF Protocol (Software-generated)
    ├── Pulse-Width Encoding
    └── Error Detection
```

---

### 2. Hardware Architecture

#### 2.1 Transmitter Hardware Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    TRANSMITTER HARDWARE                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐    ┌─────────────┐    ┌───────────────────┐   │
│  │   Switches  │    │   CH32V003  │    │   FS1000A RF      │   │
│  │  (4x Push)  │    │   MCU       │    │   Module          │   │
│  └──────┬──────┘    └──────┬──────┘    └─────────┬─────────┘   │
│         │                  │                     │              │
│         ▼                  ▼                     ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌───────────────────┐   │
│  │ GPIO Input  │    │  GPIO       │    │ RF Interface      │   │
│  │ PD0-PD3     │    │  PC1        │    │ 3-wire: VCC, GND  │   │
│  │ (Pull-up)   │    │  DATA OUT   │    │ DATA IN            │   │
│  └─────────────┘    └──────┬──────┘    └─────────┬─────────┘   │
│                            │                     │              │
│                            ▼                     ▼              │
│                   ┌─────────────┐    ┌───────────────────┐     │
│                   │ GPIO Output │    │ External EN       │     │
│                   │ PD0-PD3     │    │ Transistor/MOSFET │     │
│                   │ (Disabled)  │    │ for Power Save    │     │
│                   └─────────────┘    └───────────────────┘     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 2.2 Receiver Hardware Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     RECEIVER HARDWARE                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────┐    ┌─────────────┐    ┌───────────────────┐   │
│  │   XY-MK-5V  │    │   CH32V003  │    │   LEDs           │   │
│  │   RF Module │    │   MCU       │    │  (4x Colored)     │   │
│  └──────┬──────┘    └──────┬──────┘    └─────────┬─────────┘   │
│         │                  │                     │              │
│         ▼                  ▼                     ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌───────────────────┐   │
│  │ RF Signal   │    │ GPIO        │    │ GPIO Output       │   │
│  │ Detection   │    │ PC1         │    │ PD0-PD3           │   │
│  └──────┬──────┘    │  DATA IN    │    │ (LED Control)     │   │
│         │           └──────┬──────┘    └─────────┬─────────┘   │
│         ▼                 │                     │              │
│  ┌─────────────┐          │                     │              │
│  │ GPIO Input  │◄─────────┘                     │              │
│  │ PD0-PD3     │                              │              │
│  │ (Buttons)   │                              │              │
│  └─────────────┘                              │              │
│                                               │              │
└─────────────────────────────────────────────────────────────────┘
```

#### 2.3 Transmitter Pin Assignment Table

| MCU | Pin | Function | Mode | Notes |
|-----|-----|----------|------|-------|
| CH32V003F4P6 | PD0 | Input | GPIO | Button 1 (with internal pull-up) |
| CH32V003F4P6 | PD1 | Input | GPIO | Button 2 (with internal pull-up) |
| CH32V003F4P6 | PD2 | Input | GPIO | Button 3 (with internal pull-up) |
| CH32V003F4P6 | PD3 | Input | GPIO | Button 4 (with internal pull-up) |
| CH32V003F4P6 | PC1 | Output | GPIO | FS1000A DATA (Transmitter) |

#### 2.3 Receiver Pin Assignment Table

| MCU | Pin | Function | Mode | Notes |
|-----|-----|----------|------|-------|
| CH32V003F4P6 | PD0 | Output | GPIO | LED 1 (Red) - Current limiting resistor required |
| CH32V003F4P6 | PD1 | Output | GPIO | LED 2 (Green) - Current limiting resistor required |
| CH32V003F4P6 | PD2 | Output | GPIO | LED 3 (Blue) - Current limiting resistor required |
| CH32V003F4P6 | PD3 | Output | GPIO | LED 4 (Yellow) - Current limiting resistor required |
| CH32V003F4P6 | PC1 | Input | GPIO | XY-MK-5V DATA (Receiver) |

---

### 3. Software Architecture

#### 3.1 Software Layer Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      APPLICATION LAYER                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐ │
│  │  Transmitter    │    │  Receiver       │    │  Common     │ │
│  │  Application    │    │  Application    │    │  Utilities  │ │
│  │  Main Loop      │    │  Main Loop      │    │             │ │
│  └────────┬────────┘    └────────┬────────┘    └──────┬──────┘ │
│           │                      │                    │        │
├───────────┼──────────────────────┼────────────────────┼────────┤
│           ▼                      ▼                    ▼        │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐ │
│  │  RF Driver      │    │  RF Driver      │    │  Protocol   │ │
│  │  (FS1000A)      │    │  (XY-MK-5V)     │    │  Module     │ │
│  │                  │    │                  │    │             │ │
│  │  - Data Encoding │    │  - Packet       │    │  - Packet   │ │
│  │  - TX Control   │    │    Decoding     │    │    Format   │ │
│  │  - EN Pin       │    │  - Signal        │    │  - Checksum│ │
│  │    Management   │    │    Detection    │    │  - Address  │ │
│  └────────┬────────┘    └────────┬────────┘    └──────┬──────┘ │
│           │                      │                    │        │
├───────────┼──────────────────────┼────────────────────┼────────┤
│           ▼                      ▼                    ▼        │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐ │
│  │  GPIO Driver    │    │  GPIO Driver    │    │  Memory     │ │
│  │                  │    │                  │    │  Manager   │ │
│  │  - Switch Scan  │    │  - LED Control  │    │             │ │
│  │  - Debounce     │    │  - State Hold   │    │  - None     │ │
│  │  - Polling      │    │  - Auto Reset   │    │  (momentary)│ │
│  └────────┬────────┘    └────────┬────────┘    └──────┬──────┘ │
│           │                      │                    │        │
├───────────┼──────────────────────┼────────────────────┼────────┤
│           ▼                      ▼                    ▼        │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐ │
│  │  Timer/Watchdog │    │  Timer/Watchdog │    │  System     │ │
│  │                  │    │                  │    │  Services  │ │
│  │  - Periodic     │    │  - Signal       │    │             │ │
│  │    Polling      │    │    Debounce     │    │  - Interrupt│ │
│  │  - State        │    │  - Timing       │    │    Handler  │ │
│  │    Machine      │    │  - Timeout      │    │  - Startup │ │
│  └─────────────────┘    └─────────────────┘    └─────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 3.2 Module Interactions

```
┌─────────────────────────────────────────────────────────────────┐
│                    MODULE INTERACTION FLOW                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐                                               │
│  │  Main Loop   │                                               │
│  └──────┬───────┘                                               │
│         │                                                       │
│         ▼                                                       │
│  ┌──────────────┐                                               │
│  │ GPIO Driver  │◄───────────┐                                  │
│  └──────┬───────┘             │                                  │
│         │                     │                                  │
│         │                     │                                  │
│         ▼                     │                                  │
│  ┌──────────────┐             │                                  │
│  │  Button      │             │                                  │
│  │  Debounce    │             │                                  │
│  └──────┬───────┘             │                                  │
│         │                     │                                  │
│         │                     │                                  │
│         ▼                     │                                  │
│  ┌──────────────┐             │                                  │
│  │  RF Driver   │             │                                  │
│  └──────┬───────┘             │                                  │
│         │                     │                                  │
│         │                     │                                  │
│         ▼                     │                                  │
│  ┌──────────────┐             │                                  │
│  │  Protocol    │             │                                  │
│  │  Module      │             │                                  │
│  └──────────────┘             │                                  │
│                              │                                  │
└──────────────────────────────┼──────────────────────────────────┘
                               │
                               ▼
                    ┌───────────────────┐
                    │   RF Signal       │
                    │   Transmission    │
                    └───────────────────┘
```

---

### 4. Communication Protocol Architecture

#### 4.1 Protocol Stack Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      PROTOCOL STACK                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    APPLICATION LAYER                       │   │
│  │  - Button State Encoding (4-bit bitmap)                    │   │
│  │  - Device Addressing (8-bit)                              │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    PRESENTATION LAYER                      │   │
│  │  - Packet Formatting (4 bytes total)                      │   │
│  │  - Preamble Synchronization                               │   │
│  │  - XOR Checksum Calculation                               │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    PHYSICAL LAYER                          │   │
│  │  - Pulse-Width Encoding (Software-generated)              │   │
│  │  - 500μs HIGH + 500μs LOW = Logic 0                        │   │
│  │  - 1000μs HIGH + 500μs LOW = Logic 1                      │   │
│  │  - Direct GPIO toggling (no UART/SPI)                     │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Note:** No SPI, UART, or protocol stack layers. FS1000A/XY-MK-5V are simple ASK/OOK modules.

#### 4.2 Data Packet Structure

```
┌──────────────────────────────────────────────────────────────────┐
│                      DATA PACKET FORMAT                          │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │  PREAMBLE│  │ ADDRESS  │  │  BUTTONS │  │ CHECKSUM │          │
│  │ (1 byte) │  │ (1 byte) │  │(1 byte)  │  │ (1 byte) │          │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘          │
│                                                                  │
│  PREAMBLE:  0xAA (Synchronization pattern)                       │
│  ADDRESS:   Device ID (0-255, default = 0x00)                    │
│  BUTTONS:   4-bit bitmap for button states                       │
│             Bit 0 = Button 1, Bit 1 = Button 2, etc.             │
│  CHECKSUM:  XOR of all previous bytes                            │
│                                                                  │
│  Example Packet (Buttons 1 and 3 pressed):                       │
│  AA 42 05 ED                                                     │
│   │  │  │  │                                                     │
│   │  │  │  └─ Checksum = AA ^ 42 ^ 05 = ED                       │
│   │  │  └──── Button Bitmap: 0101 (binary) = Buttons 1,3         │
│   │  └──────── Device ID = 0x42                                  │
│   └─────────── Preamble                                          │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

#### 4.3 State Machine Architecture

**Transmitter State Machine:**
```
┌─────────────────────────────────────────────────────────────────┐
│                    TRANSMITTER STATE MACHINE                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│              ┌──────────────┐                                   │
│              │  IDLE        │◄──────────────────────────────────┤
│              │               │                                  │
│              │  (No TX)     │                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     │ Button Pressed                            │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  SCAN        │                                   │
│              │               │                                  │
│              │  Read Buttons │                                  │
│              │  Debounce     │                                  │
│              └──────┬───────┘                                   │
│                     │ Valid Press Detected                       │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  ENCODE      │                                   │
│              │               │                                  │
│              │  Format Packet│                                  │
│              │  Calc Checksum│                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     │ RF Signal Present                         │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  TRANSMIT    │                                   │
│              │               │                                  │
│              │  Send Packet  │                                  │
│              │  Every 20-30ms│                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     └───────────────────────────────────────────┘
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

**Receiver State Machine:**
```
┌─────────────────────────────────────────────────────────────────┐
│                    RECEIVER STATE MACHINE                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│              ┌──────────────┐                                   │
│              │  IDLE        │◄──────────────────────────────────┤
│              │               │                                  │
│              │  (No RF)     │                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     │ RF Signal Detected                        │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  RECEIVE     │                                   │
│              │               │                                  │
│              │  Capture Data │                                  │
│              │  Verify Chksum│                                  │
│              │  Extract Info │                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     │ Valid Packet Received                      │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  ACTIVATE    │                                   │
│              │               │                                  │
│              │  Set LED ON  │                                  │
│              │  Hold State  │                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     │ RF Signal Lost                            │
│                     ▼                                           │
│              ┌──────────────┐                                   │
│              │  RESET       │                                   │
│              │               │                                  │
│              │  Clear LED   │                                  │
│              │  After 100ms │                                  │
│              └──────┬───────┘                                   │
│                     │                                           │
│                     └───────────────────────────────────────────┘
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 4.4 Communication Strategy

The system implements a low-power, event-driven communication strategy to maximize transmitter battery life and ensure receiver responsiveness.

##### 4.4.1 Idle State
* **Transmitter:**
  * The MCU is placed in deep sleep mode to conserve power.
  * The RF transmitter hardware is kept inactive (powered off).
  * The MCU wakes up only via a GPIO interrupt triggered by a button press.
* **Receiver:**
  * The RF receiver is kept active and continuously listens to the medium.
  * The MCU runs in a polling loop waiting for valid packets.

##### 4.4.2 Button Pressed
1. The GPIO interrupt wakes the transmitter MCU from sleep.
2. The MCU reads the current states of all buttons.
3. Debouncing logic is applied to filter input noise.
4. A valid packet containing the current button state bitmap and address is generated.
5. The packet is repeatedly transmitted every 25 ms as long as any button remains pressed.

*Example transmission stream (Button 1 pressed, Device ID 0x42):*
* `AA 42 01 E9`
* `AA 42 01 E9`
* `AA 42 01 E9`

##### 4.4.3 Button Released
When all buttons are released:
1. The transmitter sends a "release packet" (button bitmap = `0x00`) several times (typically 3 times) to ensure the receiver processes the release even in the presence of RF packet loss:
   * `AA 42 00 E8`
   * `AA 42 00 E8`
   * `AA 42 00 E8`
2. Once the transmission completes, the MCU returns to deep sleep mode.

#### 4.5 Receiver State Handling

To ensure robust state management and prevent LEDs from remaining stuck in the ON state if a release packet is missed due to interference, the receiver tracks timing and state using the following variables:

```c
uint32_t last_packet_time;
uint8_t led_bitmap;
```

When a valid packet is received:
1. The LED output bitmap is updated with the received packet's bitmap:
   ```c
   led_bitmap = packet.bitmap;
   ```
2. The packet timestamp is updated to the current system time:
   ```c
   last_packet_time = millis();
   ```

A periodic task runs in the receiver's main loop to enforce an automatic timeout:
```c
if ((millis() - last_packet_time) > 100)
{
    led_bitmap = 0;
}
```

This ensures that even if the release packets are entirely lost, the LEDs will turn off automatically within 100 ms of the last successfully received transmission.

---

### 5. RF Signal Encoding

#### 5.1 Pulse-Width Encoding Scheme

The physical layer uses a custom software-defined pulse-width encoding scheme:
* **Logic 0:** 500 µs HIGH pulse followed by a 500 µs LOW pulse.
* **Logic 1:** 1000 µs HIGH pulse followed by a 500 µs LOW pulse.

```
┌─────────────────────────────────────────────────────────────────┐
│                    PULSE-WIDTH ENCODING                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Logic 0:  [HIGH 500μs] ──┬── [LOW 500μs] ──┐                   │
│                         │                     │                   │
│  Logic 1:  [HIGH 1000μs] ─┼── [LOW 500μs] ──┘                   │
│                         │                     │                   │
│                                                                  │
│  Timing Diagram:                                                 │
│                                                                  │
│  Bit Stream:   1  0  0  1  0  1  1  0                             │
│  Signal:       ┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐          │
│               │   ││   ││   ││   ││   ││   ││   ││   │          │
│  Time:         └───┴───┴───┴───┴───┴───┴───┴───┴───┘          │
│               ┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐          │
│               │   ││   ││   ││   ││   ││   ││   ││   │          │
│  HIGH:        └───┴───┴───┴───┴───┴───┴───┴───┴───┘          │
│               ┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐┌───┐          │
│               │   ││   ││   ││   ││   ││   ││   ││   │          │
│  LOW:         └───┴───┴───┴───┴───┴───┴───┴───┴───┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

#### 5.2 Transmission Method & Implementation Notes

* **Direct GPIO Toggling on PC1:** The RF transmitter is driven directly via the GPIO pin PC1. Signals are generated using software-controlled pin toggling.
* **No Serial Protocol Peripherals:** The physical layer does not use UART, SPI, or any external hardware interface controllers.
* **No External Protocol Stack:** All framing, packetization, and bit-level modulation are handled entirely in the application software.
* **Timer-Based Timing:** Software timers or interrupts are utilized to generate precise microsecond-level pulse widths.
* **Effective Baud Rate:** Approximately 1200 bps, calculated based on the nominal 500 µs unit interval.

---

### 6. Project Structure

```
rf_remote/
├── common/
│   ├── protocol.c          # Packet serialization and parsing
│   ├── protocol.h          # Packet definitions and structural layout
│   ├── checksum.c          # XOR checksum calculation
│   └── checksum.h          # Checksum helper function prototypes
├── transmitter/
│   ├── main.c              # Main application logic & MCU sleep manager
│   ├── buttons.c           # Button matrix reading and debouncing
│   ├── buttons.h
│   ├── rf_tx.c             # RF transmitter GPIO software modulation on PC1
│   └── rf_tx.h
├── receiver/
│   ├── main.c              # Main receiver application flow
│   ├── leds.c              # LED state management & auto-off timers
│   ├── leds.h
│   ├── rf_rx.c             # Software demodulator & bit sampling
│   └── rf_rx.h
└── platform/
    ├── gpio.c              # GPIO configuration helper functions
    ├── timer.c             # Timer configuration and system ticks
    └── sleep.c             # MCU low-power sleep management APIs
```

---

### 7. Technical Corrections Summary

| Issue | Original | Corrected |
|-------|----------|-----------|
| RF Interface | SPI/UART protocol stack | ASK/OOK (3-wire: VCC, GND, DATA) |
| Pin Assignments | PA8, PA9 (invalid) | PD0-PD7, PC0-PC5 (valid) |
| RF Enable Pin | Built-in EN pin | External transistor/MOSFET required |
| Packet Size | Large header+address+data | 4 bytes: Preamble + ID + Bitmap + Checksum |
| State Storage | EEPROM/Flash writes | **None** - momentary control only |
| Retransmission | ACK/retry mechanism | **Not implemented** - one-way link |
| Protocol Layers | Full OSI stack | Flat architecture, no layers |

---

### 8. Performance Specifications

| Parameter | Specification |
|-----------|---------------|
| **Packet Size** | 4 bytes (32 bits) |
| **Transmission Rate** | ~1200 bps |
| **Latency** | <100ms end-to-end |
| **Power (Idle)** | <10μA |
| **Power (Active)** | <5mA |
| **Range** | ≥30m line-of-sight |

---

### 9. Revision History

| Version | Date | Description |
|---------|------|-------------|
| 1.0 | 2026-06-01 | Initial draft with technical errors |
| 1.1 | 2026-06-01 | **Corrected** based on architectural review |
| 1.2 | 2026-06-01 | Incorporated review feedback on communication strategy, state handling, physical layer, and project structure |

---

### 10. Approval & Sign-off

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Product Manager | _____________ | _____________ | _____________ |
| Lead Engineer | _____________ | _____________ | _____________ |
| QA Lead | _____________ | _____________ | _____________ |

---
