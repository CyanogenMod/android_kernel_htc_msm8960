GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
EXTRA_CFLAGS += -Idrivers/media/video/msm
EXTRA_CFLAGS += -Idrivers/media/video/msm/io
obj-$(CONFIG_MSM_ACTUATOR) += msm_actuator.o
obj-$(CONFIG_S5K3H2YX_ACT) += s5k3h2yx_act.o
obj-$(CONFIG_S5K4E5YX_ACT) += s5k4e5yx_act.o
obj-$(CONFIG_IMX074_ACT) += imx074_act.o
obj-$(CONFIG_IMX105_ACT) += imx105_act.o
