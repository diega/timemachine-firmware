# Quick Start - Time Machine ESP32-C3

## Prerequisites
- macOS or Linux (Debian/Ubuntu)
- Python 3.8+
- Git

## Quick Steps

```bash
# 1. Install ESP-IDF v5.5.1
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git ~/esp-idf
cd ~/esp-idf
./install.sh esp32c3
. ./export.sh

# 2. Install Wokwi CLI (for testing)
curl -L https://wokwi.com/ci/install.sh | sh

# Get your Wokwi CLI token from: https://wokwi.com/dashboard/ci
export WOKWI_CLI_TOKEN="your_token_here"

# 3. Clone and build the project
cd ~/
git clone <your-repo> timemachine-firmware
cd timemachine-firmware

# Build with Wokwi configuration
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build

# Run in Wokwi simulator (WiFi + NTP fully functional)
wokwi-cli --timeout 150000 .
```

See [LOCAL_SETUP.md](LOCAL_SETUP.md) for detailed instructions.

---

## Verify It Works

### With Wokwi:
```bash
wokwi-cli --timeout 150000 .
```

You should see:
```
I (xxx) wifi:connected with Wokwi-GUEST
I (xxx) timemachine: Got IP:10.13.37.2
I (xxx) timemachine: Connected to network
I (xxx) ntp_sync: Time synchronized!
Time: 12:34
```

---

## Documentation

- **[README.md](../README.md)** - Main documentation
- **[LOCAL_SETUP.md](LOCAL_SETUP.md)** - Detailed setup guide
- **[TESTING.md](TESTING.md)** - Testing guide
- **[TIMEZONES.md](TIMEZONES.md)** - Timezone configuration
- **[CONFIGURATION.md](CONFIGURATION.md)** - Configuration system explained

---

## Common Commands

```bash
# Activate ESP-IDF environment (in each new terminal)
. ~/esp-idf/export.sh

# Configure the project
idf.py menuconfig

# Build for ESP32-C3
idf.py set-target esp32c3
idf.py build

# Flash to hardware
idf.py -p /dev/ttyUSB0 flash monitor

# Test in Wokwi simulator
wokwi-cli --timeout 150000 .
```

---

## Minimal Configuration Required

Before flashing to hardware, configure these values:

```bash
idf.py menuconfig
```

Navigate to **Time Machine Configuration**:
- **WiFi SSID**: Your WiFi network
- **WiFi Password**: Your password
- **Timezone**: Your timezone (e.g., `UTC0`, `<-03>3`, `EST5EDT,M3.2.0/2,M11.1.0`)

---

## Common Issues

### "idf.py: command not found"
```bash
. ~/esp-idf/export.sh
```

### USB port not available (Linux)
```bash
sudo usermod -a -G dialout $USER
# logout and login
```

---

## Ready!

If Wokwi tests pass, you're ready to:
1. Configure your WiFi and timezone
2. Flash to your ESP32-C3
3. See NTP-synchronized time
4. Add more features! 🚀

**Hardware needed for Phase 2:**
- ESP32-C3 (RISC-V)
- MAX7219 LED Matrix (8x32)
- Jumper wires
- 5V power supply
