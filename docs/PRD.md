# 📄 PRODUCT REQUIREMENTS DOCUMENT (PRD) - CORRECTED

## Wireless RF Control System - CH32V003F4P6 Platform

---

### 1. Executive Summary

**Project Name:** RF-Switch-Control  
**Platform:** CH32V003F4P6 Microcontroller  
**RF Modules:** FS1000A (Transmitter) + XY-MK-5V (Receiver)  
**Version:** 1.1 (Corrected)  
**Date:** 2026-06-01

---

### 2. Product Vision

Create a reliable, low-cost wireless control system that allows users to operate 4 independent push switches remotely through RF transmission, with each switch controlling a dedicated LED indicator on the receiver side. The system prioritizes simplicity, reliability, and ease of deployment.

**Key Correction:** This is a **momentary control system** - LEDs stay ON only while buttons are held down. No state persistence required.

---

### 3. Product Goals

#### 3.1 Primary Objectives
- ✅ Implement 4 independent wireless switches with dedicated LED indicators
- ✅ Achieve reliable RF communication at 433MHz frequency
- ✅ Ensure real-time response (<100ms latency) from switch press to LED activation
- ✅ Maximize battery life through MCU sleep modes and event-driven transmission.
- ✅ Provide robust debouncing for mechanical switch reliability

#### 3.2 Secondary Objectives
- 📊 Enable easy firmware updates via bootloader or GPIO selection
- 🔧 Support modular code architecture for future expansion
- 🌍 Ensure compatibility with standard CH32V003 development boards
- 💰 Keep BOM cost under $5 per complete system

---

### 4. Functional Requirements

#### 4.1 Transmitter (FS1000A Side)

| ID    | Requirement          | Priority | Description                                                 |
| ----- | -------------------- | -------- | ----------------------------------------------------------- |
| TR-01 | Switch Detection     | Critical | Detect and debounce 4 push switches on **PD0-PD3**          |
| TR-02 | RF Transmission      | Critical | Transmit button bitmap packets via FS1000A on **PC1**       |
| TR-03 | Low Power Operation  | Critical | Sleep while idle and wake on button activity                |
| TR-04 | Addressing           | Medium   | Support unique device ID (8-bit)                            |
| TR-05 | Error Checking       | High     | Implement XOR checksum validation                           |
| TR-06 | Release Notification | High     | Send multiple release packets when all buttons are released |

#### 4.2 Receiver (XY-MK-5V Side)

| ID    | Requirement       | Priority | Description                                             |
| ----- | ----------------- | -------- | ------------------------------------------------------- |
| RX-01 | RF Reception      | Critical | Decode packets received on PC1                          |
| RX-02 | LED Control       | Critical | Drive LEDs on PD0-PD3                                   |
| RX-03 | Packet Timeout    | Critical | Turn off LEDs if no valid packet received within 100 ms |
| RX-04 | Address Filtering | High     | Ignore packets from other device IDs                    |
| RX-05 | Packet Validation | High     | Verify checksum before updating outputs                 |

#### 4.3 Communication Protocol

| ID | Requirement | Priority | Description |
|----|-------------|----------|-------------|
| PR-01 | Packet Format | Critical | Define header, payload, and checksum structure |
| PR-02 | Transmission Rate | High | Optimize for reliable delivery with minimal latency |
| PR-03 | Re-transmission | **Removed** | **NOT IMPLEMENTED** - One-way link only |
| PR-04 | Interference Avoidance | High | Address-based filtering to prevent cross-talk |

---

### 5. Non-Functional Requirements

#### 5.1 Performance Requirements

| Metric               | Target                |
| -------------------- | --------------------- |
| Switch Response Time | <100 ms               |
| RF Range             | ≥30 m LOS             |
| Packet Loss Rate     | <1%                   |
| Receiver Timeout     | 100 ms                |
| Packet Interval      | 20–30 ms while active |

#### 5.2 Reliability Requirements

| Metric | Target | Test Duration |
|--------|--------|---------------|
| MTBF | ≥50,000 hours | Accelerated aging test |
| Temperature Range | -20°C to +85°C | Environmental chamber test |
| Humidity Tolerance | 10% to 90% RH | Environmental chamber test |
| Switch Bounce | <10ms jitter | Oscilloscope measurement |

#### 5.3 Usability Requirements

| Requirement | Description |
|-------------|-------------|
| **Setup Time** | <5 minutes for initial deployment |
| **User Interface** | 4 clearly labeled switches and colored LEDs |
| **Error Indication** | Visual feedback for system status |
| **Documentation** | Complete wiring diagram and code comments |

---

### 6. System Constraints

#### 6.1 Hardware Constraints
- MCU: **CH32V003F4P6** (32KB Flash, 8KB RAM)
- RF Frequency: **433MHz fixed** (FS1000A/XY-MK-5V modules)
- Power Supply: 3.3V - 5V DC (regulator required if >5V)
- Operating Temperature: -20°C to +85°C

#### 6.2 Software Constraints
- Language: C/C++ for embedded compatibility
- Compiler: GCC ARM Toolchain or IAR Embedded Workbench
- Memory Limit: ≤32KB Flash, ≤8KB RAM usage
- Real-time: **No RTOS required (bare-metal preferred)**

#### 6.3 Environmental Constraints
- Operating Temperature: -20°C to +85°C
- Humidity: 10% to 90% non-condensing
- Electromagnetic Interference: Must comply with FCC/CE regulations
- Physical Protection: IP40 minimum (dust and splash resistant)

---

### 7. User Stories

#### 7.1 Primary User Scenarios

**Story 1: Basic Operation**
> As a **system user**, I want to press a switch on the transmitter so that the corresponding LED lights up on the receiver, allowing me to control devices remotely.

**Story 2: Multiple Switches**
> As a **system user**, I want to operate all 4 switches simultaneously so that all 4 LEDs activate independently without interference.

**Story 3: Power Saving**
> As a **battery-powered transmitter**, I want the RF module to disable automatically when idle so that battery life is maximized.

**Story 4: Reliable Communication**
> As a **system integrator**, I want error checking and addressing so that the receiver ignores packets from unintended devices.

#### 7.2 Edge Cases

| Scenario                       | Expected Behavior                          |
| ------------------------------ | ------------------------------------------ |
| Rapid switch pressing          | Debouncing prevents false triggers         |
| RF interference                | Invalid packets rejected                   |
| Button release packet lost     | Receiver timeout clears LEDs within 100 ms |
| Power loss during transmission | LEDs clear after receiver timeout          |
| Multiple buttons pressed       | Bitmap supports simultaneous operation     |

---

### 8. Success Metrics

#### 8.1 Quantitative Metrics
- [ ] 99% packet delivery rate at 30m range
- [ ] <100ms end-to-end latency
- [ ] <5μA average current consumption (idle)
- [ ] Successful operation across -20°C to +85°C

#### 8.2 Qualitative Metrics
- [ ] Intuitive user interface
- [ ] Reliable performance in real-world conditions
- [ ] Easy maintenance and troubleshooting
- [ ] Scalable architecture for future features

---

### 9. Future Enhancements (Roadmap)

| Feature | Priority | Effort | Description |
|---------|----------|--------|-------------|
| RF-2.0 | Medium | High | Add support for second generation RF modules |
| Multi-transmitter | Low | Medium | Support multiple transmitters with same receiver |
| Remote Reset | Low | Low | Software reset capability via RF command |
| Data Logging | Medium | High | Log switch events to SD card or serial |
| Mobile App | Low | Very High | iOS/Android companion app for monitoring |

---

### 10. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| RF Interference | Medium | High | Address filtering, frequency hopping (future) |
| Battery Drain | Low | Medium | Software-controlled RF enable/disable |
| Switch Bounce | Low | Low | Hardware + software debouncing |
| Firmware Corruption | Low | Critical | Bootloader with recovery mode |

---

### 11. Approval & Sign-off

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Product Manager | _____________ | _____________ | _____________ |
| Lead Engineer | _____________ | _____________ | _____________ |
| QA Lead | _____________ | _____________ | _____________ |

---

**Document Control:**
- Version 1.0: Initial draft with technical errors
- Version 1.1: Corrected based on architectural review (2026-06-01)
