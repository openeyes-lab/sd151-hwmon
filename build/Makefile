sd151-hwmon-objs := sd151.o sd151_proc.o sd151_wdog.o sd151_hwm.o

obj-m += sd151-hwmon.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo cp sd151-hwmon.ko /lib/modules/$(shell uname -r)/kernel/drivers/hwmon
	sudo depmod -a

load:
	sudo insmod sd151-hwmon.ko

reload:
	sudo rmmod sd151-hwmon.ko
	sudo insmod sd151-hwmon.ko
