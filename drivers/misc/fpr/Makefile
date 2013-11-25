#
# Makefile for TI's shared transport line discipline
# and its protocol drivers (BT, FM, GPS)
#
obj-$(CONFIG_FPR_SPI) 		+= vfsSpiDrv.o

INCLUDE_DIR += -I$(PWD)

EXTRA_CFLAGS += $(INCLUDE_DIR) -DPLATFORM_BIG_ENDIAN=1 -DVCS_FEATURE_SENSOR_WINDSOR=0

#ifeq (${BUILDTYPE}, Debug)
	EXTRA_CFLAGS += -DDEBUG=0
#else
#	EXTRA_CFLAGS += -DDEBUG=0
#endif

