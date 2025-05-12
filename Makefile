obj-m += tcs34725_ioctl_driver.o
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd) modules
clean: 
	make -C $(KDIR) M=$(shell pwd) clean
run: test_TCS_ioctl.c
	gcc test_TCS_ioctl.c -o run 
	sudo ./run
	sudo rm run