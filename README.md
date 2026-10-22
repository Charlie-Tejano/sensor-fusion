# Sensor Fusion AEB Car

A bare-metal **Automatic Emergency Braking (AEB)** vehicle controller for the **STM32F407 Discovery** board. It fuses an **HC-SR04 ultrasonic rangefinder** and the on-board **LIS3DSH accelerometer** to drive a DC motor safely: it brakes for obstacles, fails safe when the sensor loses its echo, and eases off the throttle on downhill slopes.

Everything is written at the **register level in C: no hardware abstraction layer, no vendor drivers.** Every peripheral is configured with direct register writes and bit-shifts.

---

## Features

- **Interrupt-driven distance sensing.** TIM3 generates the trigger pulse and timestamps the echo edges in hardware, so ranging never blocks the CPU.
- **AEB with hysteresis.** The motor stops below 10 cm and only resumes above 15 cm, which prevents stop/start chatter at the threshold.
- **Fail-safe behavior.** A missing or stale echo, an out-of-range reading, or an e-stop press all stop the motor.
- **Distance-based speed control.** Duty cycle steps down as an obstacle gets closer.
- **Hill-descent control.** On a decline (detected from the accelerometer) the commanded duty is reduced by 5 percentage points.
- **Live 16x2 LCD readout** of distance, duty cycle, terrain and AEB state.

---

## Hardware

| Component | Role |
|---|---|
| STM32F407VG Discovery board (Cortex-M4, 16 MHz internal oscillator) | Microcontroller |
| LIS3DSH 3-axis accelerometer (on-board) | Tilt / terrain sensing over SPI |
| HC-SR04 ultrasonic module | Obstacle distance, 2-400 cm |
| H-bridge motor driver + DC motor | Actuation (PWM speed + two direction inputs) |
| HD44780 16x2 character LCD | Status display |
| Rotary encoder with push button | E-stop button (and an encoder counter, see [Known limitations](#known-limitations)) |

---

## Peripherals and their purposes

| Peripheral | Configuration | Purpose |
|---|---|---|
| **SPI1** | Master, Mode 3 (CPOL=1, CPHA=1), 8-bit, MSB first, software chip-select, fPCLK/64 = **250 kHz** | Reads accelerometer registers and configures the LIS3DSH |
| **TIM3** | 1 MHz tick (PSC=15), ARR=59999 (**60 ms** period). CH1 = PWM output, CH2 = dual-edge input capture with digital filter (IC2F = 0xF). Update + CC2 interrupts | CH1 fires the 15 us HC-SR04 trigger every 60 ms; CH2 timestamps the echo's rising and falling edges to measure pulse width |
| **TIM4** | PSC=15, ARR=999: **1 kHz** PWM, 1000 duty steps, CH4 | Motor speed control |
| **GPIO** | Push-pull outputs, inputs with pull-ups, alternate-function pins | LCD bus, motor direction, SPI chip-select, encoder/button inputs, pin muxing for SPI/TIM |
| **EXTI + SYSCFG** | EXTI1 (PB1, rising edge) and EXTI15 (PB15, falling edge) | EXTI1 counts encoder detents; EXTI15 latches the **e-stop** flag |
| **NVIC** | TIM3 priority 1, EXTI15_10 priority 1, EXTI1 priority 2 | Interrupt prioritization (e-stop and echo capture outrank the encoder) |
| **RCC** | AHB1/APB1/APB2 clock enables | Powers up GPIOA/B/D/E, SPI1, TIM3, TIM4, SYSCFG |
| **Cortex-M4F FPU** | Single-precision hardware float | Distance conversion and g-force scaling |

## Protocols and interfaces

| Protocol | Used with | Details |
|---|---|---|
| **SPI (Mode 3)** | LIS3DSH accelerometer | Bit 7 of the command byte is the read flag (`reg \| 0x80`). A read is two bytes: command, then a dummy `0xFF` while the sensor clocks the value back. Each register read is its own chip-select transaction. |
| **Pulse-width ranging** | HC-SR04 | 10 us minimum trigger pulse (15 us used); the echo pulse width is proportional to distance: `cm = width_us / 58`. |
| **PWM** | H-bridge motor driver | 1 kHz, duty 0-100%. Direction is set by two GPIO lines (DIRA=1/DIRB=0 forward, both 0 stop). |
| **HD44780 4-bit parallel** | 16x2 LCD | Bit-banged on GPIOD with busy-flag polling on DB7, using 4-bit initialization by instruction. |
| **Quadrature-style encoder** | Rotary encoder | OUTA rising edge triggers an interrupt; the OUTB level at that moment gives direction. |

---

## Pin map

| Pin | Function | Connected to |
|---|---|---|
| PA5 | SPI1_SCK (AF5) | LIS3DSH clock |
| PA6 | SPI1_MISO (AF5) | LIS3DSH data out |
| PA7 | SPI1_MOSI (AF5) | LIS3DSH data in |
| PE3 | GPIO output | LIS3DSH chip-select (active low) |
| PB4 | TIM3_CH1 (AF2) | HC-SR04 **TRIG** |
| PB5 | TIM3_CH2 (AF2), pull-down | HC-SR04 **ECHO** |
| PD15 | TIM4_CH4 (AF2) | Motor PWM |
| PD13 / PD12 | GPIO output | Motor DIRA / DIRB |
| PD7 / PD6 / PD5 | GPIO output | LCD RS / RW / EN |
| PD3 / PD2 / PD1 / PD0 | GPIO | LCD DB7 / DB6 / DB5 / DB4 |
| PB1 | EXTI1 | Encoder OUTA |
| PB0 | GPIO input | Encoder OUTB |
| PB15 | EXTI15 | Push button (**e-stop**) |

---

### Control rules

| Condition | Action |
|---|---|
| E-stop button pressed | Stop motor. **Latched**: clear with a board reset. |
| No valid echo, or echo older than 4 trigger cycles (roughly 240-300 ms), or reading beyond range | Stop motor (fail-safe) |
| Distance < 10 cm | AEB engaged: stop motor |
| AEB engaged and distance still <= 15 cm | Stay stopped (hysteresis band) |
| Distance < 30 cm | 40% duty |
| 30 to 60 cm | 60% duty |
| Above 60 cm | 80% duty |
| Accelerometer X < -0.05 g (decline) | Subtract 5 percentage points from the duty above |

### Ultrasonic driver (TIM3)

1. CH1 runs in PWM mode and fires a **15 us** pulse every **60 ms** (the HC-SR04 datasheet recommends a cycle longer than 60 ms), with no CPU involvement.
2. CH2 captures the echo's **rising edge**, then the ISR flips the polarity to capture the **falling edge**.
3. Pulse width = difference of the two timestamps (counter wraparound handled). Widths outside **116-23,200 us** (about 2-400 cm) are rejected.
4. A 16 MHz-clock digital input filter (`IC2F = 0xF`) and an echo-pin pull-down suppress spurious edges from nearby motor switching noise.
5. The main loop reads the result under a short interrupt mask so the width and its age stay consistent.

### Accelerometer (SPI1)

- Configured for **100 Hz, all axes enabled, block-data-update on, +/-2 g range** (`CTRL_REG4 = 0x6F`, `CTRL_REG5 = 0x00`).
- Each sample reads six output registers (X/Y/Z, high and low byte). The 16-bit two's-complement result is scaled at **0.06 mg/LSB**.
- Tilt is classified from the X axis with a +/-0.05 g dead band: **incline**, **decline**, or **flat**.

---

## Known limitations

- **Single narrow sensor.** The HC-SR04 has a roughly 15 degree beam, so an off-axis obstacle can be missed.
- **No obstacle means a stop.** The design deliberately fails safe, so open space beyond 4 m (no echo) halts the vehicle.
- **Tilt is a static measurement.** An accelerometer cannot tell tilt from acceleration, and the classifier has no debounce or hysteresis yet, so brief motion can flicker the terrain label near the threshold.
- **E-stop is latching.** Only a reset clears it.
- **Encoder unused in the control loop.** The rotary counter is initialized and readable through the API, but the main loop does not use it yet.
- **Motor PWM is 1 kHz,** which is audible.

## Possible improvements

- Use the LIS3DSH auto-increment burst read to replace six chip-select transactions with one (roughly halves bus time).
- Add debounce/hysteresis to the tilt classifier, mirroring the AEB hysteresis.
- Scale braking distance with closing speed instead of a fixed 10 cm threshold.
- Raise the PWM frequency out of the audible range.

---

## Author

Charlie Tejano
