# MarmoAAP Hardware Overview

This document provides detailed information about the hardware setup for the MarmoAAP apparatus, including motor control, strain gauge amplification, and integration with a Teensy microcontroller. 

---

## Microcontroller

**Recommended Model:** `Teensy 4.0`

The Teensy operates at **3.3V logic** and communicates with 5V motor components through a level shifter.

### Teensy Power Pins

- **GND** – Common ground for the entire system  
- **+5V** – Powers the level shifter and strain gauge amplifier  
- **+3.3V** – Powers the low-voltage side of the level shifter and the potentiometer  

---

## Motor Control

| Teensy Pin | Signal         | Connection Route                                  |
|------------|----------------|---------------------------------------------------|
| Pin 8      | Motor Enable   | → Level Shifter → 5V → Motor Enable pin           |
| Pin 9      | Motor Direction| → Level Shifter → 5V → Motor Direction pin        |
| Pin 10     | Motor PWM      | → Level Shifter → 5V → Motor PWM pin              |
| Pin 11     | Motor Feedback | → Level Shifter → 5V → Motor Feedback pin         |

---

## Logic Level Shifter

**Recommended Part:** [SparkFun Bi-Directional Logic Level Converter](https://www.sparkfun.com/sparkfun-logic-level-converter-bi-directional)  
**Note:** Any 3.3V ↔ 5V level shifter is suitable.

- **LV (Low-Voltage side):** Connect to `3.3V` from Teensy  
- **HV (High-Voltage side):** Connect to `5V` from Teensy  
- **Pins 8–11** from the Teensy are routed through the level shifter to the motor.

---

## Sensor Inputs to Teensy

| Teensy Pin | Input Source                 | Notes                                                 |
|------------|------------------------------|-------------------------------------------------------|
| Pin 14     | Output from INA125P (pin 10) | Analog readout from strain gauge amplifier            |
| Pin 15     | Potentiometer wiper          | Pot V+ and GND connect to Teensy `3.3V` and `GND`     |

---

## Strain Gauge Amplifier (INA125P)

**Model:** `INA125P`  
**Gain:** Set using a `30Ω resistor` between pins 8 and 9  
**Reference Voltage:** `2.5V`, typically from a voltage divider

### Pin Mapping

| INA125P Pin | Function                                  | Connection                                       |
|-------------|-------------------------------------------|--------------------------------------------------|
| 1–2         | V+                                        | +5V from Teensy                                  |
| 3           | GND                                       | Ground from Teensy                               |
| 4           | Reference voltage input                   | Connect to Pin 14 of Teensy                      |
| 5           | Analog Ground                             | Shared with Teensy GND, strain gauge GND, pin 12 |
| 6–7         | Strain gauge Vin+ / Vin−                  | Connect to load cell                             |
| 8–9         | Gain set resistor                         | Connected with 30Ω resistor                      |
| 10          | Output                                     | Connect to Pin 14 of Teensy **and** pin 11       |
| 11          | Output loop                                | Connect to pin 10                                |
| 12          | Analog ground loop                         | Connect to pin 5                                 |
| 13–16       | —                                         | Not connected                                    |

---

## Load Cell

We used a basic strain gauge-type load cell, such as this one:

🔗 [Adafruit 5kg Load Cell (Product ID: 4540)](https://www.adafruit.com/product/4540#technical-details)

- Load range: ~1kg–5kg is suitable
- Wire to the INA125P as described above
- If additional amplification is needed, consider using a dedicated HX711 amplifier

---

## Fabrication Notes

- Most components were **waterjet-cut from aluminum**
- The **potentiometer mounting plate** can be 3D printed using standard PLA filament
- If available, **metal 3D printing** could also be used for load-bearing parts

---

## Thread Adapter Tip

To bridge between imperial (#4-40) and metric (M5) threads—such as connecting an ER6 rod to a strain gauge:

🔗 [McMaster-Carr Hex Thread Adapter](https://www.mcmaster.com)  
Look for imperial-to-metric thread adapters with hex heads or through-hole mounting.

---

## Power Supply

We used an **IPC-series power supply** bundled with the ClearPath motor. We recommend:

- Use **whatever model is recommended by the motor manufacturer** (IPC-3 or IPC-5)
- The IPC-3 is likely sufficient unless your rig draws significantly more current

---

## ClearPath Servo Motor Control

- It is **technically possible** to control a ClearPath motor with just the Teensy
- We used the **ClearPath software on a PC** to configure the motor mode
- The Teensy then sent a signal to control torque dynamically
- Using a **ClearCore controller** may simplify setup, but we did not use one

---
