MODULE_NAME		:= module_project_os

ifneq ($(KERNELRELEASE),)
obj-m			:= $(MODULE_NAME).o
$(MODULE_NAME)-m	:= procmeminfo.o
$(MODULE_NAME)-m	+= util.o
ccflags-y		+= -I$(KBUILD_EXTMOD)/include
ccflags-y       += -std=gnu99  # Add this line to enable C99 mode

name-fix   = $(squote)$(quote)$(subst $(comma),_,$(subst -,_,$1))$(quote)$(squote)
ccflags-y += -DCONFIG_MODULE_NAME=$(call name-fix,$(MODULE_NAME))
else
PWD			:= $(shell pwd)
KERNEL_HEAD		:= $(shell uname -r)
KERNEL_DIR		:= /lib/modules/$(KERNEL_HEAD)/build

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

install:
	sudo insmod $(MODULE_NAME).ko

remove:
	sudo rmmod $(MODULE_NAME)

info:
	modinfo procmeminfos.ko

read:
	cat /proc/memory_info

processLog:
	dmesg

clearLog:
	sudo dmesg -C

writeALL:
	echo "ALL" > /proc/memory_info && cat /proc/memory_info

writeRESET:
	echo "RESET" > /proc/memory_info && cat /proc/memory_info

writeFILTER:
	echo "FILTER|sshd" > /proc/memory_info && cat /proc/memory_info

writeDEL:
	echo "DEL|sshd" > /proc/memory_info && cat /proc/memory_info

archive:
	ls
	cd .. && tar -zcvf group_10.tar.gz src

endif