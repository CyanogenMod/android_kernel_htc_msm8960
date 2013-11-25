#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_NFC_PN544) += pn544.o
obj-$(CONFIG_AMP_TFA9887)	+= tfa9887.o
obj-$(CONFIG_AMP_RT5501)        += rt5501.o
obj-$(CONFIG_AMP_TFA9887L)	+= tfa9887l.o
obj-$(CONFIG_AMP_TPA6185)       += tpa6185.o
obj-$(CONFIG_AMP_TPA2051D3)     += tpa2051d3.o
obj-$(CONFIG_INPUT_CAPELLA_CM3629)      += cm3629.o
obj-$(CONFIG_SENSORS_AKM8975)   += akm8975.o
obj-$(CONFIG_SENSORS_AKM8975_PANA_GYRO)   += akm8975_pana_gyro.o
obj-$(CONFIG_SENSORS_PANASONIC_GYRO)	+= ewtzmu2.o
obj-$(CONFIG_SENSORS_AK8963)    += akm8963.o
obj-$(CONFIG_SENSORS_R3GD20)    += r3gd20.o
obj-$(CONFIG_SENSORS_BMA250)	+= bma250.o
obj-$(CONFIG_BOSCH_BMA250)      += bma250_bosch.o
obj-$(CONFIG_VP_A1028)          += a1028.o
obj-$(CONFIG_TPS65200)          += tps65200.o
obj-$(CONFIG_SENSORS_BMA2X2)    += bma2x2_driver.o
obj-$(CONFIG_SENSORS_BMG160)    += bmg160_driver.o bmg160.o
obj-$(CONFIG_SENSORS_BMG160)    += bst_sensor_common.o
obj-$(CONFIG_INPUT_YAS_MAGNETOMETER) += yas53x_kernel.o
EXTRA_CFLAGS += -DCONFIG_BMA_USE_FIXED_SYSFS_PATH
EXTRA_CFLAGS += -DCONFIG_BMG_USE_FIXED_SYSFS_PATH
EXTRA_CFLAGS += -DCONFIG_BMA_USE_PLATFORM_DATA
EXTRA_CFLAGS += -DCONFIG_BMG_USE_PLATFORM_DATA
ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

