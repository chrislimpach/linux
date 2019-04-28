#
# tn40xx_ Driver
#
EXTRA_CFLAGS += -D_TN4022_DEV_ 
DRV_NAME := tn4022
DRV_OBJ  := tn40.o QT2025_phy.o  CX4.o
EXTRA_CFLAGS += -D_TN4022_DEV_ -I$(M)/$(DRV_NAME)
obj-m    += $(DRV_NAME).o
$(DRV_NAME)-objs := $(DRV_OBJ)
