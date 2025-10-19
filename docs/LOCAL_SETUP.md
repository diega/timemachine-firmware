# Local Setup Guide for ESP32-C3

Esta guía te ayuda a configurar el proyecto localmente en tu máquina para el **ESP32-C3**.

## Prerrequisitos

- macOS o Linux (Debian/Ubuntu)
- Python 3.8 o superior
- Git
- Conexión a internet

## Paso 1: Instalar ESP-IDF v5.5.1

### En macOS:

```bash
# Instalar dependencias del sistema
brew install cmake ninja dfu-util

# Clonar ESP-IDF
cd ~/
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Instalar toolchain para ESP32-C3
./install.sh esp32c3

# Activar entorno
. ./export.sh

# Agregar alias a tu shell (opcional)
echo 'alias get_idf=". ~/esp-idf/export.sh"' >> ~/.zshrc
```

### En Debian/Ubuntu:

```bash
# Instalar dependencias del sistema
sudo apt-get update
sudo apt-get install -y git wget flex bison gperf python3 python3-pip \
    python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util \
    libusb-1.0-0

# Clonar ESP-IDF
cd ~/
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# Instalar toolchain para ESP32-C3
./install.sh esp32c3

# Activar entorno
. ./export.sh

# Agregar alias a tu shell (opcional)
echo 'alias get_idf=". ~/esp-idf/export.sh"' >> ~/.bashrc
```

## Paso 2: Instalar QEMU para Testing

### En macOS:

```bash
# Instalar dependencias de QEMU
brew install libgcrypt glib pixman sdl2 libslirp

# Instalar QEMU de ESP-IDF
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

### En Debian/Ubuntu:

```bash
# Instalar dependencias de QEMU
sudo apt-get install -y libgcrypt20 libglib2.0-0 libpixman-1-0 \
    libsdl2-2.0-0 libslirp0

# Instalar QEMU de ESP-IDF
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

## Paso 3: Instalar Herramientas de Testing

```bash
cd $IDF_PATH
./install.sh --enable-pytest
```

## Paso 4: Clonar y Configurar el Proyecto

```bash
# Clonar el proyecto (si aún no lo hiciste)
git clone <tu-repo-url> timemachine-firmware
cd timemachine-firmware

# Asegurar que ESP-IDF está activo
. ~/esp-idf/export.sh

# Configurar proyecto
idf.py menuconfig
# Navegar a "Time Machine Configuration" y configurar:
# - WiFi SSID
# - WiFi Password
# - Servidores NTP
# - Timezone
```

## Paso 5: Build y Test

```bash
# Build
idf.py set-target esp32c3
idf.py build

# Test with Wokwi (WiFi + NTP functional)
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build
wokwi-cli --timeout 150000 .
```

## Paso 6: Flash a Hardware Real (Opcional)

```bash
# Conectar ESP32-C3 via USB

# Verificar puerto (puede ser diferente)
ls /dev/tty.* # macOS
ls /dev/ttyUSB* # Linux

# Flash y monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Comandos Útiles

```bash
# Activar entorno ESP-IDF (en cada terminal nueva)
. ~/esp-idf/export.sh

# Build
idf.py build

# Test with Wokwi
wokwi-cli --timeout 150000 .

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor serial
idf.py -p /dev/ttyUSB0 monitor

# Flash + Monitor
idf.py -p /dev/ttyUSB0 flash monitor

# Limpiar build
idf.py fullclean

# Configurar proyecto
idf.py menuconfig
```

## Troubleshooting

### "idf.py: command not found"
```bash
# Activar el entorno
. ~/esp-idf/export.sh
```

### "Permission denied" al acceder al puerto serial (Linux)
```bash
# Agregar usuario al grupo dialout
sudo usermod -a -G dialout $USER

# Cerrar sesión y volver a entrar
```

### "QEMU not found"
```bash
# Reinstalar QEMU
python $IDF_PATH/tools/idf_tools.py install qemu-riscv32
```

### Build falla con errores de dependencias
```bash
# Verificar versión de ESP-IDF
cd ~/esp-idf
git describe --tags

# Debería mostrar: v5.5.1

# Si es diferente, cambiar a v5.5.1
git fetch --all --tags
git checkout v5.5.1
git submodule update --init --recursive
./install.sh esp32c3
```

### Tests fallan con timeout
```bash
# Incrementar timeout en pytest.ini
timeout = 600  # 10 minutos
```

## Verificar Instalación

Corré este script para verificar que todo está instalado correctamente:

```bash
#!/bin/bash
echo "Verificando instalación..."

# ESP-IDF
if [ -z "$IDF_PATH" ]; then
    echo "❌ ESP-IDF no está activado"
else
    echo "✅ ESP-IDF: $IDF_PATH"
fi

# Python
python --version && echo "✅ Python" || echo "❌ Python"

# idf.py
which idf.py && echo "✅ idf.py" || echo "❌ idf.py"

# QEMU
$IDF_PATH/tools/idf_tools.py export --format key-value | grep QEMU && echo "✅ QEMU" || echo "❌ QEMU"

# pytest
pytest --version && echo "✅ pytest" || echo "❌ pytest"

echo ""
echo "Si todos tienen ✅, estás listo para desarrollar!"
```

## Próximos Pasos

Una vez que tengas todo funcionando:

1. Revisar [README.md](README.md) para la documentación general
2. Ver [TESTING.md](TESTING.md) para guía de testing
3. Consultar [TIMEZONES.md](TIMEZONES.md) para configurar tu timezone
4. Empezar a desarrollar! 🚀

## Referencia Rápida de Configuración

### WiFi
```
Time Machine Configuration → WiFi SSID: "tu-red"
Time Machine Configuration → WiFi Password: "tu-password"
```

### Timezone (ejemplos)
```
UTC: "UTC0"
Argentina: "<-03>3"
México (CDMX): "CST6CDT,M3.2.0/2,M11.1.0"
España: "CET-1CEST,M3.5.0,M10.5.0/3"
```

### NTP Servers
```
Primary: "pool.ntp.org"
Secondary: "time.nist.gov"
```
