#
# Tehuti Networks 10GbE Adapter Driver
# Copyright (C) 2011 Tehuti Networks Ltd.
# All rights reserved
#
#  Makefile for the tehuti Linux 2.6 Ethernet Driver
#

#
# Build Configuration
#
PWD           := $(shell pwd)
KVERSION      := 3.10.17
EXPECTED_KDIR := $(srctree)

DRV_OBJ  :=
DRV_NAME := tehuti

INSTDIR = /lib/modules/$(KVERSION)/kernel/drivers/net

#MODULE_DIR:="SUBDIRS=$(PWD)"
MODULE_DIR:=M=$(PWD)

# Set KERNELS to the list of kernels versions to build. Each version
# may be optionally terminated with % which acts as a wildcard
# character.
# i.e. KERNELS	 = 2.6.18-194.8.1.el5 2.6.34.1 2.6.35%
KERNELS	 =  2.6.18-194.% 2.6.24% 2.6.25% 2.6.26% 2.6.27% \
            2.6.28% 2.6.29% 2.6.31% 2.6.32% 2.6.33% 2.6.34% 2.6.35% 2.6.36%
VERSIONS = $(filter $(KERNELS),$(subst /lib/modules/,, \
		$(wildcard /lib/modules/2.6.*)))

#PATTERN...,TEXT)
#2.6.29% 
# Check existance of kernel build directory
#
KDIR=$(shell [ -e $(EXPECTED_KDIR) ] && echo $(EXPECTED_KDIR))
ifeq (,$(KDIR))
  $(error Aborting the build: Linux kernel $(EXPECTED_KDIR) source not found)
endif


ifneq ($(KERNELRELEASE), )
# 
# Linux kernel module build system variables
#
obj-m            += $(DRV_NAME).o
$(DRV_NAME)-objs := $(DRV_OBJ)

else
#
# 2.4.x & 2.6.x Specific rules
#
MAJOR_VERSION:=$(shell echo $(KVERSION) | cut -c1-3 | sed 's/2\.[56]/2\.6/')
ifneq ($(MAJOR_VERSION), 2.4)
MAJOR_VERSION:=2.6
endif

CURR_VERSION:=$(shell cat current.version)
DISTRO:=$(shell ./get-distro.sh)
PLATFORM:=$(shell uname -i)

ifeq ($(MAJOR_VERSION), 2.6)

#
# We're running out of simple 'make'; we should use kernel Makefile instead.
#

.PHONY: all build build-versions help clean driver driver_versions \
	install install-versions
all: build

xx:
	@echo VERSIONS $(VERSIONS)
	@echo KERNELS $(KERNELS)
	@echo MODULES $(subst /lib/modules/,, \
	$(wildcard /lib/modules/2.6.*))
	@echo $(KVERSION) $(PLATFORM) $(DISTRO)
	@echo $(CURR_VERSION)

build:
	echo Building kernel $(KVERSION)
	$(MAKE) -C $(KDIR) $(MODULE_DIR) modules  && \
	echo "$(KVERSION) $(PLATFORM) $(DISTRO)" > current.version

build-versions:
	for v in $(VERSIONS) ; \
	do \
	make R=$$v build  || \
	failed_list="$$failed_list $$v";  \
	done ; \
	if [ -n "$$failed_list" ] ; \
	then \
		echo "ERROR: $$failed_list failed" ; \
		exit 1 ; \
	fi;

clean: 
	$(MAKE) -C $(KDIR) $(MODULE_DIR) clean

help usage:
	@echo "tehuti driver Makefile. Available targets:" 
	@echo "  all              - build the driver (same as build)" 
	@echo "* build            - build the driver" 
	@echo "  build-versions   - Build multi-versions of the driver" 
	@echo "  clean            - Clean the driver"
	@echo "  driver           - Install driver and update module.aliase"
	@echo "  driver-versions  - Install drivers and update module.aliase in"
	@echo "                     the appropriate system directories"
	@echo "  help             - Print this help message"
	@echo "  install          - Install driver to system directory"
	@echo "                     usually, it is /lib/modules/VER/kernel/drivers/net"
	@echo "  install-versions - Install multi versions of the driver to"
	@echo "                     the appropriate system directories"
	@echo 
	@echo "*                  - Default target"

$(DRV_NAME).i:
	echo Building kernel $(KVERSION)
	$(MAKE) -C $(KDIR) $(MODULE_DIR) $(DRV_NAME).i  && \
	echo "$(KVERSION) $(PLATFORM) $(DISTRO)" > current.version

install: $(DRV_NAME).ko
	install -d $(INSTDIR)
	install -m 644 $(DRV_NAME).ko $(INSTDIR)

driver: install
	depmod `uname -r`

install-versions:
	for v in $(VERSIONS) ; \
	do \
        make R=$$v install  || \
	failed_list="$$failed_list $$v";  \
	done ; \
	if [ -n "$$failed_list" ] ; \
	then \
		echo "ERROR: $$failed_list failed" ; \
		exit 1 ; \
	fi;

driver-versions:
	for v in $(VERSIONS) ; \
	do \
	( make R=$$v build install && depmod $$v ) || \
	    failed_list="$$failed_list $$v";  \
	done ; \
	if [ -n "$$failed_list" ] ; \
	then \
		echo "ERROR: $$failed_list failed" ; \
		exit 1 ; \
	fi;

ifneq ($(KVERSION) $(PLATFORM) $(DISTRO), $(CURR_VERSION))
build: clean
install : build
endif
#
# Kernel 2.4
#
else # ifeq ($(MAJOR_VERSION),2.6)

# default compiler
#CC:=gcc

# standard flags for module builds
CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall
CFLAGS += -I$(KDIR)/include -I.
CFLAGS += $(shell [ -f $(KDIR)/include/linux/modversions.h ] && \
            echo "-DMODVERSIONS -DEXPORT_SYMTAB \
			-include $(KDIR)/include/linux/modversions.h")

CFLAGS += $(CFLAGS_EXTRA)

#.SILENT: $(DRV_NAME).o
$(DRV_NAME).o: $(DRV_OBJ)
	$(LD) $(LDFLAGS) -r $^ -o $@
	echo "$(DRV_NAME).o  built for $(KVERSION)"

$(DRV_OBJ): Makefile $(DRV_NAME).c $(DRV_NAME).h $(DRV_NAME)_fw.h

clean:
	rm -rf $(DRV_NAME).ko $(DRV_OBJ)

default:
	make

# 
# end of Kernel 2.4
#
endif # ifeq ($(MAJOR_VERSION),2.6)

tar:    
	./make_tar
endif
