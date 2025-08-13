# Sprinkletron

🤖 Waters my plants 🌱 🦾

A single-purpose, low-power plant watering node. It wakes from deep sleep every 24 hours, samples a capacitive soil moisture sensor, and if dryness is detected, it runs a small pump. Then it returns to deep sleep.

## Install

We will use [PlatformIO](https://platformio.org/) to build and upload the code to the ESP32. Get the VSCode extension or just install the CLI tool.

```bash
pio project init --board esp32dev
```

## Build and deploy

```bash
pio run -e esp32dev -t upload
```

## Hardware

[ESP32 microcontroller](https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z/ref=sr_1_3_pp?crid=2CQV94O1OGR6H&dib=eyJ2IjoiMSJ9.kzd_BN2te2KAhw5tyJI73iwZ9stA77zzn51rdKmEszU4feVIJwvlwVF7kBoWgQPeMgyoptu8p8PzFvEWpxs40OoVDV9nYCBotN80_zApyaZimXkio5ei2YzGSVvl4dqeCSIJUxzCsaTnyW31OWoWwsTuaZWMGaY5S9vqwavqU0O-cqKcmrz-FhhcZC4ucgOs61cPUe_J0Nli8lbT-HIqCe1nEO4xkyimmhq6HhRRjuQ.lS9xh2RYkxiqmAJ3G8zQThrpcej4UvebpuDbfTgma1s&dib_tag=se&keywords=esp32&qid=1755052045&sprefix=esp%2Caps%2C191&sr=8-3) - A low-power microcontroller with built-in Wi-Fi and Bluetooth capabilities. It is used to control the watering system.

[Breadboard power supply](https://www.amazon.com/Breadboard-Minidodoca-Alligator-Raspberry-Electronic/dp/B0BP9V6WXX/ref=sr_1_1?crid=U1BJFQH6YTKS&dib=eyJ2IjoiMSJ9.oSeKeuwdMzQHSh6XMZ9xst5nq7arbypQQPb5RQhmqlM97TPUxVJbHUCrqBxC0q58dG8GDxHlrEOWtG4X1uttjTsbqlgioDu1OsmwSiv1T-da3vqRt6rwmdx-IYf7ahkuhM0vv54AfiAK3Kq0QQeolcn7nNYEyq1vPacTNDdmzyMU7DxKe6UwdqhZNTGvS-2YYOMkUxrkH19ex5qmsdEDKaeLCkcuxfWEd9lq-dfpzUM.uC9mFgUy00We1Vr0BWzcHfaiN8h9xt-ibtmd8Q9CYbc&dib_tag=se&keywords=elegoo%2Bbread%2Bboard%2Bpower%2Bsupply&qid=1755052097&sprefix=elegoo%2Bbread%2Bboard%2Bpower%2Bsupply%2B%2Caps%2C167&sr=8-1&th=1) - A power supply module that can be plugged into a breadboard to provide power to the ESP32 and other components. It provides 3.3V and 5V outputs.

[Capacitive Soil Moisture Sensor](https://www.amazon.com/dp/B07SYBSHGX?ref=ppx_yo2ov_dt_b_fed_asin_title) - Corrosion Resistant Moisture Detection Garden Watering for Arduino DIY 3.3~5.5V

[Adafruit 4546 Submersible 3V DC](https://www.amazon.com/dp/B085KYZCDV?ref=ppx_yo2ov_dt_b_fed_asin_title) - A small 3V DC pump that can be used to water plants. a DC motor that is powered with 3V and draws 100mA. When powered, the pump sucks water in from the side of the plastic casing and pushes it out the tubing port. The pump must be primed by keeping it inside water at all times. You can PWM the motor to speed up or slow down the flow rate.

[RFP30N06LE RFP30 N-Channel Power MOSFET](https://www.amazon.com/dp/B08ZKYXN2M?ref=ppx_yo2ov_dt_b_fed_asin_title) - The MOSFET when it recieves 3.3v HIGH signal from the ESP32 will complete the pump circuit to ground allowing the pump to run. The MOSFET is used to switch the pump on and off.

[1N5819 Schottky Diode](https://www.amazon.com/dp/B0C1V6Y8ND?ref=ppx_yo2ov_dt_b_fed_asin_title) - A diode that is used to protect the MOSFET from back EMF when the pump is turned off. The cathode of the diode is connected to the +3.3V supply and the anode is connected to the pump negative terminal.

## Wiring

I used a breadboard to wire the components together. The wiring is as follows:

- Soil moisture sensor:

  - VCC → 5V
  - GND → GND
  - A0 → GPIO36 (ADC1)

- Pump:

  - VCC → 3V3
  - GND → GND
  - IN → GPIO18 (MOSFET gate)

- MOSFET:

  - Source → GND
  - Drain → Pump GND
  - Gate → (GPIO18 → 220Ω → GATE → 100kΩ pulldown to GND to keep it off at boot)

- Diode:

  - Cathode → in front of pump +3.3V
  - Anode → in front of Pump negative terminal

- ESP32:

  - GND → GND
  - power supply → 5V
  - pin GPIO18 → Pump MOSFET gate

## GPIO Pinout

<img src="./image.png" width="1400" alt="Sprinkletron GPIO Pinout">

Soil ADC: GPIO36 (input-only, ADC1)

Sensor Observations:

- In air 2.7v
- In moist soil 2.1v
- In water 1.0v

* Sensor power switch: GPIO19 (any output-capable pin).

* Pump MOSFET gate: GPIO18 (output).

# Sprinkletron — Design Doc

## Purpose

## Hardware (reference)

- ESP32-DevKit (or similar, 3.3V logic)
- Capacitive soil moisture sensor (analog output, 3.3V compatible)
- 5V DC micro pump (peristaltic preferred) + 5V supply
- Logic-level N-MOSFET (e.g., AO3400, IRLZ44N, FQP30N06L) for low-side switching
- Flyback diode across pump (e.g., 1N5819/SS14), cathode to +5V, anode to pump negative
- Gate pulldown resistor (100k) to keep pump off at boot
- Optional: P-MOSFET/high-side switch or small transistor to gate sensor VCC from ESP32 pin
- Common ground between 5V supply and ESP32 GND

## Pinning (example)

- Sensor analog out → GPIO34 (ADC1)
- Sensor VCC → switched by GPIO26 via a transistor (do not power heavy loads directly from a pin)
- Pump MOSFET gate → GPIO25 (pulldown to GND)

## Power & Sleep

- No WiFi/BT; deep sleep timer wake-up every 4 hours (`esp_sleep_enable_timer_wakeup`)
- Sensor power is off except during measurement to reduce corrosion & power
- RTC fast memory stores `last_water_us` to enforce a minimum spacing between water events

## Measurement

- On wake: power sensor, wait 200 ms, take 32 ADC readings, use median to reduce noise
- Convert RAW to % using two-point calibration: `RAW_AIR` and `RAW_WATER`
- Compare to `MOISTURE_THRESHOLD_PCT` (default 35%)

## Actuation

- If % < threshold **and** at least 12 h since last water: run pump for `PUMP_MS` (default 6 s)
- Then sleep

## Risks & Mitigations

- **Noisy sensor / false dry:** median filter + enforce min hours between waterings
- **Flooding if stuck-low sensor:** hard cap on pump run time; min interval block
- **Boot-time glitches:** pump gate pulldown resistor; initialize gate LOW early
- **Power draw:** deep sleep between checks; sensor only powered while measuring

## What it does **not** do

- No networking, no clock sync, no manual override, no fancy scheduling
- It only wakes, checks, optionally waters, then sleeps
