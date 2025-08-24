# ESP32 Internet Radio

An **ESP32-based internet radio** with touchscreen and web control, RGB LED status indicators, and support for multiple genres and streams. Control via hardware touch buttons or a web interface with dynamic buttons and status updates.

## Features

- **Touch controls**: Previous, Next, Play/Pause
- **Long press** for Volume mode and Genre selection
- **RGB LED** indicates:
  - Playing / Paused
  - Volume mode
  - Genre mode
  - WiFi status
- **Web interface**:
  - Round, colorful buttons for all controls
  - Real-time status display (channel, genre, volume, playing)
  - AJAX updates every second
- **Multiple genres supported**:
  - British, Australian, Groups, Decades, Comedy, Other
- **Dynamic JSON-based stream loading** from GitHub

## Hardware Required

- ESP32 module (any variant with I2S support)
- I2S audio output (DAC or amplifier)
- Capacitive touch buttons (GPIO 32, 33, 34)
- RGB LED (common anode recommended)
- Optional legacy LEDs for status

## Pin Configuration

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

## Installation

1. Clone this repository to your Arduino projects folder:

# ESP32 Internet Radio

An **ESP32-based internet radio** with touchscreen and web control, RGB LED status indicators, and support for multiple genres and streams. Control via hardware touch buttons or a web interface with dynamic buttons and status updates.

## Features

- **Touch controls**: Previous, Next, Play/Pause
- **Long press** for Volume mode and Genre selection
- **RGB LED** indicates:
  - Playing / Paused
  - Volume mode
  - Genre mode
  - WiFi status
- **Web interface**:
  - Round, colorful buttons for all controls
  - Real-time status display (channel, genre, volume, playing)
  - AJAX updates every second
- **Multiple genres supported**:
  - British, Australian, Groups, Decades, Comedy, Other
- **Dynamic JSON-based stream loading** from GitHub

## Hardware Required

- ESP32 module (any variant with I2S support)
- I2S audio output (DAC or amplifier)
- Capacitive touch buttons (GPIO 32, 33, 34)
- RGB LED (common anode recommended)
- Optional legacy LEDs for status

## Pin Configuration

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

## Installation

1. Clone this repository to your Arduino projects folder:

```bash
git clone https://github.com/YourUsername/ESP32-Internet-Radio.git

