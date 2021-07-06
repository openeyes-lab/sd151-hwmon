# SD151-HWMON - Hardware monitor Linux driver

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL%20v3-blue.svg)](http://www.gnu.org/licenses/lgpl-3.0)

This repository contains Linux drivers for the hardware monitor module implemented
on PI-POW devices from OPEN-EYES S.r.L.
This driver is licensed under the Gnu Public License.
This driver is tested under the Linux 5.X kernels.

For more information about PI-POW devices visit https://www.open-eyes.it/pipow/

The sd151-hwmon Linux driver dialogues with firmware on ATMEL ATTINY817 MCU mounted
on the PI-POW HAT

## Manual installation

### Build instructions

Prepare Raspberry for build:
```
sudo apt update
sudo apt upgrade
sudo apt-get install raspberrypi-kernel-headers git
```
Download from git:
```
git clone https://github.com/openeyes-lab/sd151-hwmon.git
```
build driver
```
cd sd151_hwmon/build
make
make install
```

### Implement device Tree overlay

source file : dts/sd151-hwmon.dts

compile dts file and copy into /boot/overlays directory
```
dtc -@ -b 0 -I dts -O dtb -o sd151-hwmon.dtbo dts/sd151-hwmon.dts
```
change compiled file owner and move it into /boot/overlays directory
```
sudo chown root:root sd151-hwmon.dtbo
sudo mv sd151-hwmon.dtbo /boot/overlays
```
add this line
```
dtoverlay=sd151-hwmon
```
into the file /boot/config.txt

reboot

## Automatic install/uninstall

After cloning the file;
to install driver execute:
```
cd sd151_hwmon
bash install.sh
```
to uninstall execute:
```
cd sd151_hwmon
bash uninstall.sh
```

## Interface involved

The firmware on the MCU implement a SLAVE I2C interface and answer to the
address 0x35.

## Filesys

HWMON is created into /sys/class/hwmon/hwmon0...x directory
RTC is created into /sys/class/rtc/rtc0...x directory
WDOG is created into /sys/class/watchdog/watchdog0...x directory

## Reference

### HWMON
https://www.kernel.org/doc/html/latest/hwmon/hwmon-kernel-api.html
https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface

#### lm-sensors

Install:
```
sudo apt install lm-sensors
```

get value with command:
```
sensors
```

### WATCHDOG

https://www.kernel.org/doc/html/latest/watchdog/watchdog-api.html
https://www.kernel.org/doc/Documentation/watchdog/watchdog-kernel-api.txt
https://www.kernel.org/doc/Documentation/watchdog/watchdog-api.txt

#### Testing
In order to test watchdog functionality:
```
cd test
make
```
This command sets a short timeout and high polling, in order to get the watchdog
running out.
```
sudo ./wdog -d -t 10 -p 1000 -e
```
The result must be a SOC reboot after 10 sec, then 2 consecutive reboots after
90 sec.

### RTC

no reference....

### POWER CONTROL

Different power down operation:

reboot or shutdown -r

From chip side, no operation on reset or power rail. at reboot the driver
detects the RESTART.
```
shudown -P (power off)
```
After 10s the info is received, the chip shutdown enables and the power rail goes
in low power mode.
Only hardware wakeup signal or power cycle can restart RPI.
```
shutdown -H (HALT)
```
The MCU is freezed.
Only power cycle can restart RPI

shutdown with wakeup:
```
sudo sh -c "echo +60 > /sys/class/rtc/rtc0/wakealarm"
sudo shutdown -P now
```
The RPI shuts down and restarts after 60s
