#!/bin/bash

echo "Running script $0 with params $1"

function upgrade_system {
	if [ "$upgrade" = false ]; then
		return
	fi
	sudo apt update
	sudo apt upgrade -y
	upgrade=false
}


if [ "$1" == "noupgrade" ]; then
	echo "Running script $0 without upgrade"
	upgrade=false
else
	echo "Running script $0 with upgrade"
	upgrade=true
fi


echo "Checking headers installation"
dpkg-query -s raspberrypi-kernel-headers
if [ $? -eq 1 ]
then
	upgrade_system
	sudo apt install -y raspberrypi-kernel-headers
	#echo "After reboot relaunch install"
	#sudo reboot
fi

echo "Checking dkms installation"
dpkg-query -s dkms
if [ $? -eq 1 ]
then
	sudo apt install -y dkms
	#echo "After reboot relaunch install"
	#sudo reboot
fi

# build overlay dtbo

if dtc -@ -b 0 -I dts -O dtb -o sd151-hwmon.dtbo dts/sd151-hwmon.dts ; then
	sudo chown root:root sd151-hwmon.dtbo
	sudo mv sd151-hwmon.dtbo /boot/overlays
else
	echo "fail to compile dts"
	exit -1
fi

if grep -q "dtoverlay=sd151-hwmon" /boot/config.txt ; then
	echo "confi.txt already prepared"
else
	echo "dtoverlay=sd151-hwmon" | sudo tee -a /boot/config.txt
fi

# Install udev rules
if [ ! -f "/etc/udev/rules.d/70-power-switch.rules" ]; then
        sudo touch /etc/udev/rules.d/70-power-switch.rules    
fi

echo "# Match the special power key input/event" | sudo tee -a /etc/udev/rules.d/70-power-switch.rules  
echo 'ACTION=="add", SUBSYSTEM=="input", ATTRS{name}=="sd151", TAG+="power-switch"' | sudo tee -a /etc/udev/rules.d/70-power-switch.rules

# Install dkms
uname_r=$(uname -r)
src="build"
mod="sd151"
ver="1.0"

if [[ -e /usr/src/$mod-$ver || -e /var/lib/dkms/$mod/$ver ]]; then
  echo "Warning previous install exist...removing!"
  sudo dkms remove --force -m $mod -v $ver --all
  sudo rm -rf /usr/src/$mod-$ver
fi

sudo mkdir -p /usr/src/$mod-$ver
sudo cp -a $src/* /usr/src/$mod-$ver/

sudo dkms add -m $mod -v $ver
sudo dkms build $uname_r -m $mod -v $ver
sudo dkms install --force $uname_r -m $mod -v $ver

echo "sd151-hwmon correctly installed: reboot to make effective"
