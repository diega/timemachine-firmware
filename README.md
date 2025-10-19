## About
Time Machine is an esp-idf implementation for a WiFi-connected clock based on ESP32-C3.
This is heavily inspired by [mfactory-osaka/ESPTimeCast](https://github.com/mfactory-osaka/ESPTimeCast) but without using the Arduino SDK.

## Architecture

The firmware uses an event-driven architecture with clear separation of concerns:

### Core Components

- **events**: Central event system defining TIMEMACHINE_EVENT and DISPLAY_EVENT event bases
- **network**: WiFi connectivity with automatic reconnection, emits NETWORK_CONNECTED/NETWORK_FAILED events
- **ntp_sync**: NTP time synchronization, emits NTP_SYNCED event when time is set
- **panel_manager**: Panel manager framework for future extensibility
- **clock_panel**: Time formatting and display logic with internal timer, emits RENDER_SCENE events
- **display**: Display abstraction layer with MAX7219 LED matrix driver

### Display System

The display system uses a scene-based approach where components build `display_scene_t` objects containing:
- Scene elements (text, bitmaps, animations)
- Fallback text for simple displays

Display drivers receive RENDER_SCENE events and render according to their capabilities.

**Display hardware**:
- **MAX7219 LED Matrix**: 4 cascaded MAX7219 modules (32x8 pixels)
  - Hardware pins: CLK=GPIO6, MOSI=GPIO7, CS=GPIO10
  - Uses default bitmap font for rendering text
  - Optional mock implementation for development without hardware (CONFIG_TIMEMACHINE_DISPLAY_EMULATOR)
  - Supports scene-based rendering with text centering

## Features
- Event-driven architecture using ESP-IDF event system
- WiFi connectivity with configurable credentials
- NTP time synchronization with configurable servers
- Timezone support
- 12h/24h time format with optional seconds
- MAX7219 LED matrix display (4 cascaded modules, 32x8 pixels)
- Multi-font support system for flexible text rendering
- Optional LED matrix emulator for development without hardware
- Modular display driver architecture

## Requirements
- ESP-IDF v5.5.1 or later
- Python 3.8+
- Wokwi CLI (for testing without hardware - has full WiFi support)

## Dependencies Installation

### Install ESP-IDF v5.5.1
```bash
# Clone ESP-IDF
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install for ESP32-C3 (RISC-V architecture)
./install.sh esp32c3

# Activate the environment (run this in every new terminal)
. ./export.sh
```

## Building and Flashing

### Configure the project
```bash
idf.py menuconfig
# Configure under "Time Machine Configuration":
# - WiFi credentials (SSID/Password)
# - NTP servers and timezone
# - Time format (12h/24h) and whether to show seconds
```

### Build for ESP32-C3
```bash
idf.py set-target esp32c3
idf.py build
```

### Flash to hardware
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Testing with Wokwi Simulator

### Wokwi Advantages

Wokwi provides a complete ESP32-C3 simulation with **full WiFi support**:
- ✅ **WiFi is fully supported** - Connects to simulated "Wokwi-GUEST" network
- ✅ **NTP synchronization works** - Real internet connectivity
- ✅ **No hardware needed** - Test complete functionality from command line
- ✅ **Fast iteration** - No flashing, instant simulation

### Prerequisites

```bash
# 1. Install Wokwi CLI
curl -L https://wokwi.com/ci/install.sh | sh

# 2. Get a Wokwi CLI token (free for open source)
# Sign up at https://wokwi.com and get your token from:
# https://wokwi.com/dashboard/ci

# 3. Export your token
export WOKWI_CLI_TOKEN="your_token_here"
```

### Run Full WiFi + NTP Test in Wokwi

```bash
# Build with Wokwi configuration
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build

# Run in Wokwi simulator (WiFi + NTP fully functional)
wokwi-cli --timeout 150000 .

# You should see:
# I (xxx) wifi:connected with Wokwi-GUEST
# I (xxx) timemachine: Got IP:10.13.37.2
# I (xxx) timemachine: Connected to network
# I (xxx) ntp_sync: Time synchronized!
# I (xxx) clock: 12:34:56
```

### Testing on Real Hardware

For testing on actual ESP32-C3:
```bash
# Configure your WiFi credentials
idf.py menuconfig
# Navigate to: Time Machine Configuration -> WiFi SSID/Password

# Build and flash to ESP32-C3
idf.py build flash monitor

# The firmware will:
# 1. Connect to your WiFi network
# 2. Sync time via NTP
# 3. Display formatted time on LED matrix
```

### Why not QEMU?

QEMU is not used for this project because it lacks WiFi support. Since Time Machine requires WiFi connectivity for NTP synchronization, QEMU cannot provide meaningful testing. For testing without hardware, use Wokwi instead (see above), which provides full WiFi and NTP functionality.

## Project Structure
```
timemachine-firmware/
├── components/
│   ├── display/            # Display abstraction with MAX7219 driver
│   │   ├── include/display.h
│   │   ├── display.c
│   │   └── max7219/        # MAX7219 LED matrix driver
│   │       ├── display_max7219.c
│   │       ├── default_font.c  # Bitmap font
│   │       └── mock/       # Mock implementation for development
│   ├── events/             # Event definitions and registration
│   │   ├── include/timemachine_events.h
│   │   ├── include/scene.h
│   │   └── timemachine_events.c
│   ├── i18n/               # Internationalization support
│   │   ├── include/i18n.h
│   │   └── i18n.c
│   ├── network/            # WiFi connectivity
│   │   ├── include/network.h
│   │   └── network.c
│   ├── ntp_sync/           # NTP synchronization
│   │   ├── include/ntp_sync.h
│   │   └── ntp_sync.c
│   ├── panel_manager/      # Panel navigation coordinator
│   │   ├── include/panel_manager.h
│   │   └── panel_manager.c
│   ├── panels/             # Panel implementations
│   │   └── clock_panel/    # Clock display panel
│   │       ├── include/clock_panel.h
│   │       └── clock_panel.c
│   └── settings/           # Settings persistence (NVS)
│       ├── include/settings.h
│       └── settings.c
├── main/
│   ├── main.c              # Application entry point
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild   # Configuration options
└── pytest/
    └── test_integration.py # Integration tests
```

## Troubleshooting

### Build fails
Ensure ESP-IDF environment is activated:
```bash
. $IDF_PATH/export.sh
```
