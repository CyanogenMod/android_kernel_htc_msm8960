GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
EXTRA_CFLAGS += -Idrivers/media/video/msm
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
EXTRA_CFLAGS += -Idrivers/media/video/msm/csi
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor.o
obj-$(CONFIG_IMX074) += imx074_v4l2.o
obj-$(CONFIG_S5K4E5YX) += s5k4e5yx_v4l2.o
ifeq ($(CONFIG_MSM_CAMERA_V4L2),y)
obj-$(CONFIG_S5K3H2YX) += s5k3h2yx_v4l2.o
obj-$(CONFIG_IMX105) += imx105_v4l2.o
obj-$(CONFIG_S5K6AAFX) += s5k6aafx_v4l2.o s5k6aafx_reg_valentewx.o
else
obj-$(CONFIG_S5K3H2YX) += s5k3h2yx.o
obj-$(CONFIG_S5K6AAFX) += s5k6aafx.o
endif
obj-$(CONFIG_S5K6A1GX) += s5k6a1gx_v4l2.o
obj-$(CONFIG_OV8830) += ov8830.o
obj-$(CONFIG_MT9D015) += mt9d015.o mt9d015_reg.o
obj-$(CONFIG_OV2720) += ov2720.o


obj-$(CONFIG_MACH_HOLIDAY) += s5k3h2yx_reg_holiday.o s5k6aafx_reg_holiday.o s5k6aafx_reg_holiday_cob.o
obj-$(CONFIG_MACH_VIGOR) += s5k3h2yx_reg_vigor.o ov8830_reg_vigor.o
obj-$(CONFIG_MACH_RUNNYMEDE) += s5k3h2yx_reg_runnymede.o s5k6aafx_reg_runnymede_cob.o
