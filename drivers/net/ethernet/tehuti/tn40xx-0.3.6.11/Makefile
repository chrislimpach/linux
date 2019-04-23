#
# tn40xx_ Driver
#
# Makefile for the tx40xx Linux driver

#######################################
# Build Configuration:
#######################################

PWD := $(shell pwd)
KVERSION := 3.10.17
EXPECTED_KDIR := $(srctree)
INSTDIR = /lib/modules/$(KVERSION)/kernel/drivers/net
MODULE_DIR:=M=$(PWD)
#
# Check existance of kernel build directory
#
KDIR=$(shell [ -e $(EXPECTED_KDIR) ] && echo $(EXPECTED_KDIR))
ifeq (,$(KDIR))
  $(error Aborting the build: Linux kernel $(EXPECTED_KDIR) source not found)
endif

DRV_NAME := tn40xx
DRV_OBJ  := tn40.o MV88X3120_phy.o QT2025_phy.o CX4.o
EXTRA_CFLAGS += -D_TN40xx_DEV_ -I$(M)/$(DRV_NAME)
obj-m    += $(DRV_NAME).o
$(DRV_NAME)-objs := $(DRV_OBJ)

.PHONY: help clean install uninstall

all: clean
	@echo Building kernel $(KVERSION)
	$(MAKE) -C $(KDIR) $(MODULE_DIR) modules 

clean: 
	$(MAKE) -C $(KDIR) $(MODULE_DIR) clean

help usage:
	@echo "tn40xx driver Makefile. Available targets:" 
	@echo "  all              - build the driver" 
	@echo "  clean            - Clean the driver"
	@echo "  help             - Print this help message"
	@echo "  install          - Install driver to system directory"
	@echo "                     usually, it is /lib/modules/VER/kernel/drivers/net"
	@echo "  default          - all"
	@echo

install: $(DRV_NAME).ko
	install -d $(INSTDIR)
	install -m 644 $(DRV_NAME).ko $(INSTDIR)
	@test ! -d /etc/pm/config.d || echo 'SUSPEND_MODULES=$(DRV_NAME)' > /etc/pm/config.d/$(DRV_NAME) 
	depmod $(KVERSION)


uninstall:
	rm $(INSTDIR)/$(DRV_NAME).ko 
	rm /etc/pm/config.d/$(DRV_NAME)
	depmod $(KVERSION)


