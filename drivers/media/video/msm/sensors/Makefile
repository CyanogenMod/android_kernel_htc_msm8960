GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
EXTRA_CFLAGS += -Idrivers/media/video/msm
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
EXTRA_CFLAGS += -Idrivers/media/video/msm/csi
EXTRA_CFLAGS += -Idrivers/media/video/msm/yushanII
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor.o
obj-$(CONFIG_OV5647) += ov5647_v4l2.o
obj-$(CONFIG_IMX091) += imx091_v4l2.o
obj-$(CONFIG_IMX175) += imx175_v4l2.o
obj-$(CONFIG_MACH_ELITE) += imx175_2lane_v4l2.o
obj-$(CONFIG_MACH_JET) += imx175_2lane_v4l2.o
obj-$(CONFIG_IMX135) += imx135_v4l2.o
ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
obj-$(CONFIG_OV4688) += ov4688_non_v4l2.o
obj-$(CONFIG_VD6869) += vd6869_non_v4l2.o
else
obj-$(CONFIG_OV4688) += ov4688_v4l2.o
obj-$(CONFIG_VD6869) += vd6869_v4l2.o
endif
obj-$(CONFIG_OV8838) += ov8838_v4l2.o
obj-$(CONFIG_S5K3H2YX) += s5k3h2yx_v4l2.o
obj-$(CONFIG_S5K6A1GX) += s5k6a1gx_v4l2.o
obj-$(CONFIG_AR0260) += ar0260_v4l2.o
obj-$(CONFIG_IMX074) += imx074_v4l2.o
obj-$(CONFIG_S5K3L1YX) += s5k3l1yx.o
obj-$(CONFIG_OV2720) += ov2720.o
obj-$(CONFIG_MT9M114) += mt9m114_v4l2.o
obj-$(CONFIG_S5K4E1) += s5k4e1_v4l2.o
obj-$(CONFIG_MT9E013) += mt9e013_v4l2.o
obj-$(CONFIG_WEBCAM_OV9726) += ov9726_v4l2.o
obj-$(CONFIG_WEBCAM_OV7692_QRD) += ov7692_qrd_v4l2.o
obj-$(CONFIG_OV2722) += ov2722_v4l2.o
obj-$(CONFIG_OV5693) += ov5693_v4l2.o
obj-$(CONFIG_S5K6A2YA) += s5k6a2ya_v4l2.o
obj-$(CONFIG_MT9V113) += mt9v113_v4l2.o mt9v113_reg_lens_9251.o
obj-$(CONFIG_S5K6AAFX) += s5k6aafx_v4l2.o s5k6aafx_reg_valentewx.o

