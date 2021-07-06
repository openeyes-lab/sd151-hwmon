sudo rm /boot/overlays/sd151-hwmon.dtbo

sudo sed -i -e "/sd151/d" /boot/config.txt

sudo rm /lib/modules/$(uname -r)/kernel/drivers/hwmon/sd151-hwmon.ko

mod="sd151"
ver="1.0"

sudo dkms remove --force -m $mod -v $ver --all

sudo rm -rf /usr/src/$mod*
