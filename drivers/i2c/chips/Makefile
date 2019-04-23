#
# Makefile for Thecus I2C chip drivers.
#

obj-y += ipmi_kernel_intf.o pca9532.o pca9532_id.o pca9532_od.o

obj-y += thecus_board.o

obj-$(CONFIG_SENSORS_N16000_IO)  += n16000_io.o
n16000_io-objs +=  n16000_gpio.o n16000_board.o

obj-$(CONFIG_SENSORS_N8900_IO)  += n8900_io.o
n8900_io-objs +=  n8900_gpio.o n8900_board.o

obj-$(CONFIG_SENSORS_N2800_IO) += n2800_io.o
n2800_io-objs +=  n2800_gpio.o n2800_board.o

obj-$(CONFIG_SENSORS_N7700PRO_IO) += n7700pro_io.o
n7700pro_io-objs +=  n7700pro_gpio.o n7700pro_board.o

obj-$(CONFIG_SENSORS_N5810_IO) += n5810_io.o
n5810_io-objs +=  n5810_gpio.o n5810_board.o

obj-$(CONFIG_SENSORS_N8950_IO) += n8950_io.o
n8950_io-objs +=  n8950_gpio.o n8950_board.o

obj-$(CONFIG_SENSORS_NXXXX_IO) += nxxxx_io.o
nxxxx_io-objs +=  braswell_gpio.o nxxxx_board.o

