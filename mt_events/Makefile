ifneq ($(KERNELRELEASE),)
	mtevents-y += events2.o event_list.o time_proc.o
	obj-m += mtevents.o
else
	KERNELRELEASE ?= /home/vyacheslav/code/kernel/linux-4.19.281
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) modules
endif