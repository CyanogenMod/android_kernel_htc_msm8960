GCC_VERSION      := $(shell $(CONFIG_SHELL) $(PWD)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)
obj-$(CONFIG_RAWCHIP) += rawchip_spi.o rawchip.o Yushan_API.o Yushan_Platform_Specific.o Yushan_HTC_Functions.o
obj-$(CONFIG_RAWCHIP) += yushan_reg.o yushan_reg_s5k3h2yx.o yushan_reg_imx175.o
