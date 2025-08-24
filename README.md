# ESP32 Internet Radio

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A **feature-rich ESP32 Internet Radio** with hardware touch buttons, web control interface, RGB LED status indicators, and dynamic genre support. Stream your favorite stations directly from your ESP32 using I2S audio output.

---

## Features

- **Touch Controls**
  - Prev / Next / Play-Pause
  - Long press for Volume or Genre mode
- **RGB LED Indicators**
  - Playing / Paused / WiFi / Volume / Genre / Channel changes
- **Web Interface**
  - Round, colorful buttons for control
  - AJAX updates every second for live status
- **Dynamic Genres & Streams**
  - British, Australian, Groups, Decades, Comedy, Other
  - JSON-based stream configuration loaded from GitHub
- **Stable Streaming**
  - Configurable audio buffer size
  - WiFi sleep disabled for uninterrupted playback

---

## Hardware Required

| Function           | GPIO  |
|-------------------|-------|
| I2S DOUT           | 25    |
| I2S BCLK           | 27    |
| I2S LRC            | 26    |
| Touch Next         | 32    |
| Touch Previous     | 33    |
| Touch Play/Pause   | 34    |
| RGB Red            | 12    |
| RGB Green          | 13    |
| RGB Blue           | 14    |
| Status LED         | 4     |
| Volume Mode LED    | 2     |

**Optional:** Amplifier, DAC, legacy status LEDs.

---

## Installation

1. Clone this repository:

```bash
git clone https://github.com/YourUsername/ESP32-Internet-Radio.git
ithub.com/YourUsername/ESP32-Internet-Radio.git

