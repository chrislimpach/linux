#!/usr/bin/make
# Makefile for building Linux QLogic 10-Gigabit ethernet driver as a module.

KVER = 3.14.48
ifeq ($(KVER),)
    KVER = $(shell uname -r)
endif

__ARCH=x86_64
KERNEL_DIR=$(srctree)

# PREFIX may be set by the RPM build to set the effective root.
PREFIX =

ifeq ($(LINUXSRC),)
   LINUXSRC=$(KERNEL_DIR)
endif

# paths
ifeq ($(shell ls /lib/modules/$(KVER)/updates > /dev/null 2>&1 && echo 1),1)
    DRV_DIR = updates
else
    ifeq ($(shell grep -q "search.*[[:space:]]updates" /etc/depmod.conf > /dev/null 2>&1 && echo 1),1)
        DRV_DIR = updates
    else
        ifeq ($(shell grep -q "search.*[[:space:]]updates" /etc/depmod.d/* > /dev/null 2>&1 && echo 1),1)
            DRV_DIR = updates
        else
            DRV_DIR = kernel/drivers/net
        endif
    endif
endif

MAN_DIR = $(PREFIX)/usr/share/man/man4

IDLE_CHK_C = bnx2x_self_test.c
REG_DUMP_H = bnx2x_dump.h
REG_HSI_H = bnx2x_reg.h bnx2x_fw_defs.h bnx2x_hsi.h bnx2x_mfw_req.h
CNIC_INC = ../../bnx2/src/cnic_if.h
INIT_OPS_H = bnx2x_init.h bnx2x_init_ops.h
SP_VERBS = bnx2x_sp.c bnx2x_sp.h
HW_CHANNEL_H = bnx2x_vfpf.h

SOURCES_PF = bnx2x_main.c bnx2x_cmn.[ch] bnx2x_link.c bnx2x.h bnx2x_link.h bnx2x_compat.h $(INIT_OPS_H) bnx2x_fw_file_hdr.h bnx2x_dcb.[ch] $(SP_VERBS) bnx2x_stats.[ch] bnx2x_ethtool.c $(IDLE_CHK_C) bnx2x_sriov.[ch] bnx2x_vfpf.c
INIT_VAL_C = bnx2x_init_values_e1.c bnx2x_init_values_e1h.c bnx2x_init_values_e2.c

CHAR_FILES_C = bnx2x_char_dev.c
CHAR_FILES_H = bnx2x_char_dev.h bnx2x_user_iface.h
CHAR_FILES = $(CHAR_FILES_C) $(CHAR_FILES_H)

MODULE_NAME = bnx2x
ifeq ($(NPA),1)
	override EXTRA_CFLAGS += -D__VMKNPA__
endif

override EXTRA_CFLAGS += -DDRV_MODULE_NAME=\"$(MODULE_NAME)\"

obj-m +=  $(MODULE_NAME).o
bnx2x-objs = bnx2x_main.o bnx2x_link.o bnx2x_cmn.o $(INIT_VAL_C:%.c=%.o) bnx2x_dcb.o bnx2x_sp.o bnx2x_stats.o bnx2x_ethtool.o bnx2x_self_test.o bnx2x_sriov.o bnx2x_vfpf.o

ifeq ($(DISABLE_WERROR),)
        override EXTRA_CFLAGS += -Werror
endif

ifeq ($(NX2_BOOT_DISK),1)
	override EXTRA_CFLAGS += -DBNX2X_DRIVER_DISK
endif

sles_distro := $(wildcard /etc/SuSE-release)
ifneq ($(sles_distro),)
SLES_VERSION = $(shell cat /etc/SuSE-release | grep VERSION | grep -o -P [0-9]+)
SLES_PATCHLEVEL = $(shell cat /etc/SuSE-release | grep PATCHLEVEL | grep -o -P [0-9]+)
PADDED_PATCHLEVEL = $(shell if [ 10 -gt $(SLES_PATCHLEVEL) ]; then echo 0$(SLES_PATCHLEVEL); else echo $(SLES_PATCHLEVEL); fi)
SLES_DISTRO_VER = "0x$(SLES_VERSION)$(PADDED_PATCHLEVEL)"
override EXTRA_CFLAGS += -DSLES_DISTRO=$(SLES_DISTRO_VER)
endif

ifeq ($(shell grep eth_zero_addr $(LINUXSRC)/include/linux/etherdevice.h > /dev/null 2>&1 && echo eth_zero_addr),)
	override EXTRA_CFLAGS += -D_DEFINE_ETH_ZERO_ADDR_
endif

ifeq ($(shell grep pcie_capability_read_word $(LINUXSRC)/include/linux/pci.h > /dev/null 2>&1 && echo pcie_capability_read_word),)
	override EXTRA_CFLAGS += -D_DEFINE_PCIE_CAPABILITY_READ_WORD_
endif

ifeq ($(shell grep tcp_v6_check $(LINUXSRC)/include/net/ip6_checksum.h > /dev/null 2>&1 && echo tcp_v6_check),)
	override EXTRA_CFLAGS += -D_DEFINE_TCP_V6_CHECK_
endif

ifeq ($(shell grep "prandom_bytes" $(LINUXSRC)/include/linux/random.h > /dev/null 2>&1 && echo prandom_bytes),)
	override EXTRA_CFLAGS += -D_DEFINE_PRANDOM_BYTES_
endif

ifeq ($(shell grep "pci_vfs_assigned" $(LINUXSRC)/include/linux/pci.h > /dev/null 2>&1 && echo pci_vfs_assigned),)
	override EXTRA_CFLAGS += -D_DEFINE_PCI_VFS_ASSIGNED_
endif

ifneq ($(shell grep -A3 "pci_driver_rh" $(LINUXSRC)/include/linux/pci.h | grep "sriov_configure" > /dev/null 2>&1 && echo pci_driver_rh),)
	override EXTRA_CFLAGS += -DHAS_SRIOV_PCI_DRIVER_RH
endif

ifneq ($(shell grep "ndo_fix_features" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo pci_driver_rh),)
	override EXTRA_CFLAGS += -DHAS_NDO_FIX_FEATURES
endif

ifeq ($(shell grep "pcie_get_minimum_link" $(LINUXSRC)/include/linux/pci.h > /dev/null 2>&1 && echo pcie_get_minimum_link),)
	override EXTRA_CFLAGS += -D_DEFINE_PCIE_GET_MIN_LINK_
endif

ifeq ($(shell ls $(LINUXSRC)/include/linux/aer.h > /dev/null 2>&1 && echo aer),)
	override EXTRA_CFLAGS += -D_DEFINE_AER
endif

ifeq ($(shell ls $(LINUXSRC)/include/linux/netdev_features.h > /dev/null 2>&1 && echo netdev_features),)
	override EXTRA_CFLAGS += -D_DEFINE_NETDEV_FEATURES_T
endif

ifeq ($(shell grep "pci_wait_for_pending_transaction" $(LINUXSRC)/include/linux/pci.h > /dev/null 2>&1 && echo pci_wait_for_pending_transaction),)
	override EXTRA_CFLAGS += -D_DEFINE_PCIE_WAIT_PEND_TRANS_
endif

ifeq ($(shell grep "netdev_phys_port_id" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo netdev_phys_port_id),)
	override EXTRA_CFLAGS += -D_DEFINE_PHYS_PORT_ID
endif

ifeq ($(shell grep "netif_get_num_default_rss_queues" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo netif_get_num_default_rss_queues),)
	override EXTRA_CFLAGS += -D_DEFINE_NETIF_GET_NUM_DEFAULT_RSS_QUEUES
endif

ifeq ($(shell grep "pci_enable_msix_range" $(LINUXSRC)/include/linux/pci.h > /dev/null 2>&1 && echo pci_enable_msix_range),)
	override EXTRA_CFLAGS += -D_DEFINE_PCI_ENABLE_MSIX_RANGE
endif

ifeq ($(shell grep "SET_ETHTOOL_OPS" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo SET_ETHTOOL_OPS),)
	override EXTRA_CFLAGS += -D_DEFINE_SET_ETHTOOL_OPS
endif

ifeq ($(shell grep "IFLA_VF_RATE" $(LINUXSRC)/include/uapi/linux/if_link.h > /dev/null 2>&1 && echo IFLA_VF_RATE),)
	override EXTRA_CFLAGS += -D_DEFINE_IFLA_VF_RATE
endif

ifeq ($(shell grep "EPROBE_DEFER" $(LINUXSRC)/include/linux/errno.h > /dev/null 2>&1 && echo EPROBE_DEFER),)
	override EXTRA_CFLAGS += -D_DEFINE_EPROBE_DEFER
endif

ifneq ($(shell grep "SKBTX_HW_TSTAMP" $(LINUXSRC)/include/linux/skbuff.h > /dev/null 2>&1 && echo ptp_clock_kernel),)
	override EXTRA_CFLAGS += -D_HAS_HW_TSTAMP
endif

ifeq ($(shell grep "__smp_mb__before_atomic" $(LINUXSRC)/include/linux/bitops.h > /dev/null 2>&1 && echo __smp_mb__before_atomic),)
	override EXTRA_CFLAGS += -D_DEFINE_SMP_MB_BEFORE_ATOMIC
endif

ifeq ($(shell grep "ether_addr_equal" $(LINUXSRC)/include/linux/etherdevice.h > /dev/null 2>&1 && echo ether_addr_equal),)
	override EXTRA_CFLAGS += -D_DEFINE_ETHER_ADDR_EQUAL
endif

ifeq ($(shell grep "ndo_busy_poll" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo ndo_busy_poll),)
	override EXTRA_CFLAGS += -D_DEFINE_LL_RX_POLL
endif

ifeq ($(shell grep "skb_set_hash" $(LINUXSRC)/include/linux/skbuff.h > /dev/null 2>&1 && echo skb_set_hash),)
	override EXTRA_CFLAGS += -D_DEFINE_SKB_SET_HASH
endif

ifeq ($(shell grep "netdev_name" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo netdev_name),)
	override EXTRA_CFLAGS += -D_DEFINE_NETDEV_NAME
endif

ifeq ($(shell grep "netdev_tx_completed_queue" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo netdev_tx_completed_queue),)
	override EXTRA_CFLAGS += -D_DEFINE_NETDEV_TX_COMPLETED_QUEUE
endif

ifneq ($(shell grep -B3 accel_priv $(LINUXSRC)/include/linux/netdevice.h | grep ndo_select_queue > /dev/null 2>&1 && echo has_accel_priv),)
	override EXTRA_CFLAGS += -DBNX2X_SELECTQUEUE_HAS_ACCEL_PARAM
endif
ifneq ($(shell grep -B4 fallback $(LINUXSRC)/include/linux/netdevice.h | grep ndo_select_queue > /dev/null 2>&1 && echo has_accel_priv),)
	override EXTRA_CFLAGS += -DBNX2X_SELECTQUEUE_HAS_FALLBACK_PARAM
endif

ifneq ($(shell grep "ethtool_ops_ext" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo ethtool_ops_ext),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_OPS_EXT
endif

ifneq ($(shell grep -B50 "\*get_rxfh_indir_size" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_rxfh_indir_size),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_RXF_INDIR_SIZE
endif

ifneq ($(shell grep -B50 "\*get_rxfh_indir" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_rxfh_indir),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_RXF_INDIR
endif

ifneq ($(shell grep -B50 "\*set_rxfh_indir" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo set_rxfh_indir),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_SET_RXF_INDIR
endif

ifneq ($(shell grep -B50 "\*get_channels" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_channels),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_CHANNELS
endif

ifneq ($(shell grep -B50 "\*set_channels" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo set_channels),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_SET_CHANNELS
endif

ifneq ($(shell grep -B50 "\*get_dump_flag" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_dump_flag),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_DUMP_FLAG
endif

ifneq ($(shell grep -B50 "\*get_dump_data" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_dump_data),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_DUMP_DATA
endif

ifneq ($(shell grep -B50 "\*set_dump" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo set_dump),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_SET_DUMP
endif

ifneq ($(shell grep -B50 "\*get_module_info" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_module_info),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_MODULE_INFO
endif

ifneq ($(shell grep -B50 "\*get_module_eeprom" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_module_eeprom),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_MODULE_EEPROM
endif

ifneq ($(shell grep -B50 "\*set_phys_id" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo set_phys_id),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_SET_PHYS_ID
endif

ifneq ($(shell grep -B50 "\*get_eee" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_eee),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_GET_EEE
endif

ifneq ($(shell grep -B50 "\*set_eee" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo set_eee),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_SET_EEE
endif

ifneq ($(shell grep -B50 "\*get_ts_info" $(LINUXSRC)/include/linux/ethtool.h | sed 's/[ ,\t]//gi' | grep structethtool_ops_ext > /dev/null 2>&1 && echo get_ts_info),)
	override EXTRA_CFLAGS += -D_HAS_ETHTOOL_EXT_TS_INFO
endif

ifneq ($(shell grep "ndo_set_vf_link_state" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo ndo_set_vf_link_state),)
	override EXTRA_CFLAGS += -D_HAS_SET_VF_LINK_STATE
endif

ifneq ($(shell grep "clamp" $(LINUXSRC)/include/linux/kernel.h > /dev/null 2>&1 && echo clamp),)
	override EXTRA_CFLAGS += -D_HAS_CLAMP
endif

ifneq ($(shell ls $(LINUXSRC)/include/net/dcbnl.h  > /dev/null 2>&1 && echo getnumtcs),)
ifneq ($(shell grep "getnumtcs" $(LINUXSRC)/include/net/dcbnl.h | cut -d'*' -f1 | grep int > /dev/null 2>&1 && echo getnumtcs),)
	override EXTRA_CFLAGS += -D_GETNUMTCS_RETURNS_INT
endif
endif

ifneq ($(shell grep "ndo_fix_features" $(LINUXSRC)/include/linux/netdevice.h > /dev/null 2>&1 && echo ndo_fix_features),)
	override EXTRA_CFLAGS += -DHAS_NDO_FIX_FEATURES
endif

rh_distro := $(wildcard /etc/redhat-release)
ifneq ($(rh_distro),)
XENSERVER_EXIST = $(shell cat /etc/redhat-release | grep XenServer)
ifneq ($(XENSERVER_EXIST),)
XENSERVER_DISTRO_MAIN = $(shell cat /etc/redhat-release | grep -o -P [0-9]+ | head --lines=1)
XENSERVER_DISTRO_SUB = $(shell cat /etc/redhat-release | grep -o -P [0-9]+ | head --lines=2 | tail --lines=1)
XENSERVER_DISTRO_SUBB = $(shell cat /etc/redhat-release | grep -o -P [0-9]+ | head --lines=3 | tail --lines=1)
XENSERVER_DISTRO_VER = $(shell echo "$(XENSERVER_DISTRO_MAIN) * 65536 + $(XENSERVER_DISTRO_SUB) * 256 + $(XENSERVER_DISTRO_SUBB)" | bc)
override EXTRA_CFLAGS += -DXENSERVER_DISTRO=$(XENSERVER_DISTRO_VER)
endif
endif

ifeq ($(SOE),1)
override EXTRA_CFLAGS += -DBNX2X_STOP_ON_ERROR
endif

ifeq ($(NSTS),1)
override EXTRA_CFLAGS += -DBNX2X_NO_STATS
endif


ifeq ($(CHAR_BUILD),1)
override EXTRA_CFLAGS += -DBNX2X_CHAR_DEV
bnx2x-objs += $(CHAR_FILES_C:%.c=%.o)
SOURCES_PF += $(CHAR_FILES)
endif

# targets and rules
.PHONY: all modules_install docs_install clean install

all default: $(obj-m)

soe:
	$(MAKE) SOE=1

nsts:
	$(MAKE) NSTS=1

char:
	$(MAKE) CHAR_BUILD=1

$(obj-m): $(SOURCES_PF) $(INIT_VAL_C) $(REG_DUMP_H) $(REG_HSI_H) $(CNIC_INC) $(HW_CHANNEL_H)
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` modules

modules_install: $(obj-m)
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` INSTALL_MOD_PATH=$(PREFIX) INSTALL_MOD_DIR=$(DRV_DIR) modules_install
	@if [ "$(PREFIX)" = "" ] ;\
		then /sbin/depmod -a ;\
		else echo " *** Run '/sbin/depmod -a' to update the module database." ;\
	fi

docs_install:
	gzip -c bnx2x.4 > bnx2x.4.gz
	install -m 644 bnx2x.4.gz $(MAN_DIR)

clean:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` clean
	@rm -f *.fw *.ihex

install: modules_install docs_install

# generate the include files
-include Makefile.gen
