## About
Time Machine is an esp-idf implementation for a WiFi-connected LED matrix clock based on ESP32-C3 and MAX7219.
This is heavily inspired by [mfactory-osaka/ESPTimeCast](https://github.com/mfactory-osaka/ESPTimeCast) but without using the Arduino SDK.

## Architecture

The firmware uses an event-driven architecture with clear separation of concerns:

### Core Components

- **events**: Central event system defining TIMEMACHINE_EVENT and DISPLAY_EVENT event bases
- **network**: WiFi connectivity with automatic reconnection, emits NETWORK_CONNECTED/NETWORK_FAILED events
- **ntp_sync**: NTP time synchronization, emits NTP_SYNCED event when time is set
- **panel_manager**: Coordinates panel navigation and inactivity timeout, listens to INPUT_TOUCH and TIME_TICK
- **touch_sensor**: TTP223 capacitive touch sensor driver, emits INPUT_TOUCH events
- **clock**: Time formatting and display logic, listens to TIME_TICK and panel events, emits RENDER_SCENE events
- **display**: Display abstraction layer with pluggable drivers, listens to RENDER_SCENE events
- **tick_task**: Generates periodic TIME_TICK events

### Display System

The display system uses a scene-based approach where components build `display_scene_t` objects containing:
- Scene elements (text, bitmaps, animations)
- Fallback text for simple displays

Display drivers receive RENDER_SCENE events and render according to their capabilities.

**Available drivers**:
- **Console**: Text output to serial terminal (for debugging)
- **LED Matrix Emulator**: Visual console output showing LED matrix state
- **MAX7219 Hardware**: 4 cascaded MAX7219 LED matrix modules (32x8 pixels)
  - Hardware pins: CLK=GPIO6, MOSI=GPIO7, CS=GPIO10
  - Uses mFactory bitmap font for rendering text
  - Supports scene-based rendering with text centering

## Features
- Event-driven architecture using ESP-IDF event system
- Panel-based display system with touch navigation (extensible for future panels like weather, date, etc.)
- TTP223 capacitive touch sensor for panel switching
- Automatic return to default panel after inactivity timeout (configurable)
- WiFi connectivity with configurable credentials
- NTP time synchronization with configurable servers
- Timezone support
- 12h/24h time format with optional seconds
- MAX7219 LED matrix display (4 cascaded modules, 32x8 pixels)
- LED matrix emulator for testing without hardware
- Console output for debugging
- Modular display driver architecture

## Requirements
- ESP-IDF v5.5.1 or later
- Python 3.8+
- Wokwi CLI (for testing without hardware - has full WiFi support)

## Dependencies Installation

### 1. Install ESP-IDF v5.5.1
```bash
# Clone ESP-IDF
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Install for ESP32-C3 (RISC-V architecture)
./install.sh esp32c3

# Activate the environment (run this in every new terminal)
. ./export.sh
```

### 2. Install QEMU for ESP32

**macOS:**
```bash
# Install system dependencies
brew install libgcrypt glib pixman sdl2 libslirp

# Install QEMU binaries for ESP32
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

**Debian/Ubuntu:**
```bash
# Install system dependencies
sudo apt-get install -y libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0

# Install QEMU binaries for ESP32
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

### 3. Install Python Testing Dependencies
```bash
# Install pytest and ESP-IDF testing tools
cd $IDF_PATH
./install.sh --enable-pytest
```

## Building and Flashing

### Configure the project
```bash
idf.py menuconfig
# Configure under "Time Machine Configuration":
# - WiFi credentials (SSID/Password)
# - NTP servers and timezone
# - Touch sensor GPIO pin (default: GPIO 5)
# - Touch debounce time (default: 200ms)
# - Panel inactivity timeout (default: 15 seconds)
# - Display driver (Console / LED Matrix Emulator / MAX7219 Hardware)
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
# 3. Display formatted time on LED matrix (or console if that driver is selected)
```

### QEMU Testing (Not Supported)

**Note:** QEMU is not used for this project because it lacks WiFi support. Since Time Machine requires WiFi connectivity for NTP synchronization, QEMU cannot provide meaningful testing beyond basic boot verification.

For comprehensive testing without hardware, use Wokwi instead (see above), which provides full WiFi and NTP functionality.

If you still want to run basic boot tests in QEMU (WiFi/NTP will not work):

```bash
# Build with QEMU test configuration
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci" build

# Run pytest with QEMU
pytest --target esp32c3 -v

# Note: QEMU tests only verify boot sequence, not WiFi/NTP functionality
# Use Wokwi for full networking tests
```

## Project Structure
```
timemachine-firmware/
├── components/
│   ├── clock/              # Time formatting and display logic
│   │   ├── include/clock.h
│   │   └── clock.c
│   ├── display/            # Display abstraction with pluggable drivers
│   │   ├── include/display.h
│   │   ├── display.c
│   │   └── drivers/
│   │       ├── console/    # Console driver (serial output)
│   │       └── max7219/    # MAX7219 LED matrix driver
│   │           ├── display_max7219.c
│   │           ├── mfactory_font.c  # Bitmap font
│   │           └── mock/   # Emulator (visual console output)
│   ├── events/             # Event definitions and registration
│   │   ├── include/timemachine_events.h
│   │   ├── include/display_scene.h
│   │   └── timemachine_events.c
│   ├── network/            # WiFi connectivity
│   │   ├── include/network.h
│   │   └── network.c
│   ├── ntp_sync/           # NTP synchronization
│   │   ├── include/ntp_sync.h
│   │   └── ntp_sync.c
│   ├── panel_manager/      # Panel navigation coordinator
│   │   ├── include/panel_manager.h
│   │   └── panel_manager.c
│   ├── tick_task/          # Periodic tick event generator
│   │   ├── include/tick_task.h
│   │   └── tick_task.c
│   └── touch_sensor/       # TTP223 touch sensor driver
│       ├── include/touch_sensor.h
│       └── touch_sensor.c
├── main/
│   ├── main.c              # Application entry point
│   ├── CMakeLists.txt
│   └── Kconfig.projbuild   # Configuration options
└── pytest/
    └── test_integration.py # Integration tests
```

## Troubleshooting

### QEMU not found
Make sure you've installed QEMU tools and system dependencies:
```bash
# Install QEMU for RISC-V (ESP32-C3)
python $IDF_PATH/tools/idf_tools.py install qemu-riscv32

# macOS: Install system dependencies
brew install libgcrypt glib pixman sdl2 libslirp

# Debian/Ubuntu: Install system dependencies
sudo apt-get install -y libgcrypt20 libglib2.0-0 libpixman-1-0 libsdl2-2.0-0 libslirp0
```

### pytest not found
Install ESP-IDF testing dependencies with the correct flag:
```bash
cd $IDF_PATH
./install.sh --enable-pytest
```
**Note:** The flag is `--enable-pytest`, not `--enable-ci`

### Tests hang in QEMU
If tests hang after "NVS initialized", ensure you're using the CI configuration:
```bash
# Rebuild with test configuration
rm -rf build sdkconfig
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.ci" reconfigure build
pytest --target esp32c3 -v
```

### Build fails
Ensure ESP-IDF environment is activated:
```bash
. $IDF_PATH/export.sh
```
