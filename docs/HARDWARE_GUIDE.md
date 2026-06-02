# 🔌 CH32V003 RF System: Hardware & Wiring Guide

This guide details the physical assembly, bill of materials (BOM), antenna design, and hardware design considerations for both the **Transmitter Node** and **Receiver Node**.

---

## 📊 Bill of Materials (BOM)

A complete transmitter-receiver pair can be constructed for **under $4.00 USD**, making this system highly cost-effective for mass-production.

| Component | Description | Qty | Target Unit Cost | Total Cost | Est. Supplier |
| :--- | :--- | :---: | :---: | :---: | :--- |
| **CH32V003F4P6 Development Board** | TSSOP20 breakout with LDO (3.3V) & debug pins | 2 | $0.80 | $1.60 | AliExpress/LCSC |
| **FS1000A RF Module** | 433.92 MHz ASK Transmitter Module | 1 | $0.40 | $0.40 | AliExpress/Amazon |
| **XY-MK-5V RF Module** | 433.92 MHz ASK Superregenerative Receiver | 1 | $0.50 | $0.50 | AliExpress/Amazon |
| **Tactile Push Buttons** | Momentary switches, through-hole (12x12mm or 6x6mm) | 4 | $0.05 | $0.20 | Generic |
| **LEDs (Red, Green, Blue, Yellow)** | 5mm T-1 3/4 through-hole indicator LEDs | 4 | $0.05 | $0.20 | Generic |
| **Metal Film Resistors (220Ω)** | 1/4W 1% LED current-limiting resistors | 4 | $0.01 | $0.04 | Generic |
| **Ceramic Capacitor (100nF)** | 50V decoupling capacitor (0.1µF) | 2 | $0.02 | $0.04 | LCSC |
| **Electrolytic Capacitor (10µF)** | 16V decoupling capacitor | 2 | $0.03 | $0.06 | LCSC |
| **Solid Core Copper Wire** | ~17.3 cm length wire for 433MHz antennas | 2 | $0.05 | $0.10 | Generic |
| **BOM TOTAL** | **Complete System Cost** | - | - | **$3.14** | - |

---

## 📡 Antenna Design (Critical for RF Range)

The FS1000A and XY-MK-5V modules do not ship with pre-soldered antennas; instead, they have labeled pads (`ANT`). Operating without an antenna reduces the range to less than 2 meters. For optimal line-of-sight range (≥30 meters), you must construct a **quarter-wave monopole whip antenna**.

### 1. Calculation
* The speed of light ($c$) is approximately $3 \times 10^8$ m/s.
* The operating frequency ($f$) of the modules is $433.92$ MHz ($433.92 \times 10^6$ Hz).
* Wavelength ($\lambda$):
  $$\lambda = \frac{c}{f} = \frac{3 \times 10^8}{433.92 \times 10^6} \approx 0.6913\text{ meters} \approx 69.13\text{ cm}$$
* Quarter-wave antenna length ($\frac{\lambda}{4}$):
  $$L = \frac{69.13\text{ cm}}{4} \approx 17.28\text{ cm}$$

### 2. Physical Construction
1. Cut **two pieces** of solid-core hookup wire (22 AWG or similar) to exactly **17.3 cm**.
2. Strip about 2mm of insulation off one end of each wire.
3. Solder one wire to the `ANT` pad on the FS1000A transmitter board.
4. Solder the other wire to the `ANT` pad on the XY-MK-5V receiver board.
5. **Orientation:** Keep the whip antennas as straight and vertical as possible. Do not wrap them around the board or run them directly parallel to power rails, as this will shift the impedance match and degrade range.
6. **Optional Compact Design:** For small enclosures, you can wind the wire into a helical coil around a 4mm drill bit (approx. 20 turns) to reduce physical size. This reduces optimal range by ~30% but saves significant space.

---

## 🔌 Transmitter Wiring Diagram

The transmitter utilizes active-low momentary buttons. Since internal pull-ups are enabled in software on `PC4-PC7`, external pull-up resistors are **not** required.

```
                  TRANSMITTER WIRING (3.3V OR 5V POWER)
                  
                   ┌───────────────────────────────┐
                   │        CH32V003F4P6           │
                   │      Development Board        │
                   └───────────┬──────┬────────────┘
         PC4   PC5   PC6   PC7 │      │ PC1
          │     │     │     │  │      │
          ├──┐  ├──┐  ├──┐  ├──┐      │
         [B1]  [B2]  [B3]  [B4]       │
          └──┬  └──┬  └──┬  └──┬      ▼
             ▼     ▼     ▼     ▼    ┌───────────────┐
             ───────────────────    │ FS1000A TX    │
                      │             ├───────────────┤
                      ▼             │ DATA   [PC1] ◀┘
                     GND            │ VCC    [VCC] ◀─── VCC (3.3V-5V)
                                    │ GND    [GND] ◀─── GND
                                    │ ANT    ──────────▶ 17.3cm Antenna
                                    └───────────────┘

* Note: B1, B2, B3, B4 are tactile push-buttons connected between GPIO and GND.
* Note: PD1 is dedicated exclusively as the SWIO programming pin and should be routed to a header.
```

---

## 🔌 Receiver Wiring Diagram

The receiver controls four LEDs. Because the MCU outputs push-pull High logic (~3.3V or 5V), current-limiting resistors are placed in series with each LED to prevent burning out the MCU pins or the diodes.

```
                    RECEIVER WIRING (5V RECOMMENDED)
                    
                   ┌───────────────────────────────┐
                   │        CH32V003F4P6           │
                   │      Development Board        │
                   └────┬─────┬─────┬─────┬────┬───┘
         PC4   PC5   PC6│  PC7│     │     │    │ PC1
          │     │     │ │  │  │     │     │    │
         [R1]  [R2]  [R3]│ [R4]     │     │    │
          ▼     ▼     ▼ ▼  ▼        │     │    ▲
         (R)   (G)   (B)(Y)         │     │  ┌───────────────┐
         LED   LED   LED LED        │     │  │ XY-MK-5V RX   │
          ▼     ▼     ▼   ▼         │     │  ├───────────────┤
          ─────────────────         │     │  │ DATA   ───────▶ [PC1]
                  │                 │     │  │ VCC    ◀─────── 5V Supply
                  ▼                 ▼     ▼  │ GND    ◀─────── GND
                 GND               VCC   GND │ ANT    ────────▶ 17.3cm Antenna
                                    │     │  └───────────────┘
                                   ┌┴─────┴┐
                                   │ C1 10u│  (Decoupling Capacitors)
                                   │ C2.1u │
                                   └───────┘

* Note: R1-R4 are 220Ω series resistors. LED colors: R=Red, G=Green, B=Blue, Y=Yellow.
* C1 (10µF) and C2 (0.1µF) should be soldered directly across the VCC and GND pins of the receiver.
* Note: PD1 is dedicated exclusively as the SWIO programming pin and should be routed to a header.
```

---

## 🔋 Power Management & Signal Conditioning

### 1. The Zero-Power ASK Transmitter Trick
Standard ASK RF transmitters (like the FS1000A) consist of a simple RF power oscillator driven directly by the `DATA` pin.
* When `DATA` is **HIGH**, the oscillator turns ON and transmits a 433MHz carrier wave.
* When `DATA` is **LOW**, the oscillator is fully disabled.
* Because of this architecture, **the FS1000A draws less than 1 µA of leakage current when the DATA line is held LOW**.
* By ensuring `PC1` is written to `Bit_RESET` (0V) before putting the CH32V003 into Standby mode, the system achieves an active RF transmitter shutdown without needing a high-side load switch (MOSFET) to disconnect VCC. This saves BOM cost, board area, and pin count.

### 2. Power Decoupling & RF Noise Mitigation
The XY-MK-5V receiver is a superregenerative receiver which uses an internal analog amplifier to boost minute 433MHz signals. This circuit is extremely sensitive to power line noise.
* **Problem:** Digital switching noise on the MCU's internal bus (specifically the 48MHz CPU clock and high-speed GPIO toggling) propagates back through the VCC rail, appearing as EMI (Electromagnetic Interference) on the RF input. This causes the receiver's AGC to clamp down, dramatically reducing range.
* **Solution:**
  - Place a **10 µF electrolytic or tantalum capacitor** in parallel with a **100 nF ceramic capacitor** directly across the VCC and GND pins of the XY-MK-5V module.
  - If possible, power the XY-MK-5V from a **5.0V regulator** and the CH32V003 from a **3.3V regulator**. The additional voltage level separation and regulator isolation will shield the receiver's front-end from digital switching transients, yielding the maximum possible range.
