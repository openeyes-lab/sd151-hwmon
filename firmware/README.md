# Linux updi stand alone programmer

This program allow to download a new firmware on Microchip ATTiny MCU

# Prerequisite

Installed Wiring Pi

```
sudo apt install wiringpi
```

reference: http://wiringpi.com/reference/setup/


# Build instructions

Prepare system for build:

```
sudo apt update
sudo apt upgrade
sudo apt-get install raspberrypi-kernel-headers
```


```
cd firmware
make
```

the application should be executed as root with highest priority

```
su && nice --20 ./sd151upgrade
``` 
