# Linux updi stand alone programmer

This program is derived from:

https://github.com/Polarisru/updiprog.git

and

https://github.com/mraardvark/pyupdi.git

but doesn't use an UART but directly drive a single GPIO

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

Download from git:

```
git clone http://server.local/git/LinuxTool/updi_programmer.git
```

```
cd updi_programmer
make
```

