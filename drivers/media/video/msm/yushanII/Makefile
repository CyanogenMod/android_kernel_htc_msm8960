GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)

EXTRA_CFLAGS += -Idrivers/media/video/msm/sensors
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
EXTRA_CFLAGS += -Idrivers/media/video/msm

obj-$(CONFIG_RAWCHIPII) += file_operation.o rawchip_spi.o yushanII.o ilp0100_customer_iqsettings.o ilp0100_customer_platform.o ilp0100_customer_sensor_config.o ilp0100_ST_core.o ilp0100_ST_debugging.o

