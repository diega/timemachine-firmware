# Testing Guide

## Quick Start

### 1. Build the project
```bash
idf.py set-target esp32c3
idf.py build
```

### 2. Test with Wokwi Simulator

Wokwi provides complete ESP32-C3 simulation with **full WiFi and NTP support**.

```bash
# Build with Wokwi configuration
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build

# Run in Wokwi simulator
wokwi-cli --timeout 150000 .
```

## Test Scenarios

The `test_firmware.py` script verifies basic firmware functionality in Wokwi:

```bash
# Run test with pytest
pytest --target esp32c3 -v
```

### Expected Test Flow

1. **Component Initialization**: Verifies that all components (NTP sync, time display) initialize correctly
2. **WiFi Connection**: Connects to Wokwi-GUEST network
3. **NTP Sync**: Synchronizes time via NTP
4. **Time Display**: Displays formatted time output

## Expected Output

When running successfully in Wokwi, you should see:

```
I (xxx) wifi:connected with Wokwi-GUEST
I (xxx) timemachine: Got IP:10.13.37.2
I (xxx) timemachine: Connected to network
I (xxx) ntp_sync: Time synchronized!
I (xxx) time_display: Display initialized with console driver
Time: 12:34
```

## Troubleshooting

### Test fails with "pytest: command not found"
```bash
cd $IDF_PATH
./install.sh --enable-pytest
```

### Wokwi CLI not found
```bash
# Install Wokwi CLI
curl -L https://wokwi.com/ci/install.sh | sh

# Set your token (get from https://wokwi.com/dashboard/ci)
export WOKWI_CLI_TOKEN="your_token_here"
```

### Build fails with component not found
Make sure you're in the project root directory and ESP-IDF environment is activated:
```bash
. $IDF_PATH/export.sh
```

## Running on Hardware

If you want to test on real hardware instead of Wokwi:

```bash
# Configure WiFi credentials
idf.py menuconfig
# Navigate to: Time Machine Configuration

# Build and flash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## QEMU Testing (Not Supported)

**Note:** QEMU is not used for this project because it lacks WiFi support. Since Time Machine requires WiFi connectivity for NTP synchronization, QEMU cannot provide meaningful testing beyond basic boot verification.

For comprehensive testing without hardware, use Wokwi instead (see above), which provides full WiFi and NTP functionality.
