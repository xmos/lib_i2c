# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to
# the project name

APP_NAME = 

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
#
#    XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
#
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to
# xcc for the final link (mapping) stage.

COMMON_FLAGS = -O2 -g -save-temps -DDEBUG_PRINT_ENABLE=1 -report

# The TARGET variable determines what target system the application is
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling.

XCC_FLAGS_400_stop_xs2        = $(COMMON_FLAGS) -DSPEED=400 -DSTOP=1
XCC_FLAGS_400_stop_xcoreai    = $(COMMON_FLAGS) -DSPEED=400 -DSTOP=1
XCC_FLAGS_400_no_stop_xs2     = $(COMMON_FLAGS) -DSPEED=400 -DSTOP=0
XCC_FLAGS_400_no_stop_xcoreai = $(COMMON_FLAGS) -DSPEED=400 -DSTOP=0
XCC_FLAGS_100_stop_xs2        = $(COMMON_FLAGS) -DSPEED=100 -DSTOP=1
XCC_FLAGS_100_stop_xcoreai    = $(COMMON_FLAGS) -DSPEED=100 -DSTOP=1
XCC_FLAGS_100_no_stop_xs2     = $(COMMON_FLAGS) -DSPEED=100 -DSTOP=0
XCC_FLAGS_100_no_stop_xcoreai = $(COMMON_FLAGS) -DSPEED=100 -DSTOP=0
XCC_FLAGS_10_stop_xs2         = $(COMMON_FLAGS) -DSPEED=10 -DSTOP=1
XCC_FLAGS_10_stop_xcoreai     = $(COMMON_FLAGS) -DSPEED=10 -DSTOP=1
XCC_FLAGS_10_no_stop_xs2      = $(COMMON_FLAGS) -DSPEED=10 -DSTOP=0
XCC_FLAGS_10_no_stop_xcoreai  = $(COMMON_FLAGS) -DSPEED=10 -DSTOP=0

XCC_FLAGS_tx_only_no_stop_xcoreai   = $(COMMON_FLAGS) -DENABLE_TX=1 -DENABLE_RX=0 -DSPEED=400 -DSTOP=0

ifeq ($(findstring xs2, $(CONFIG)), xs2)
  TARGET = XCORE-200-EXPLORER
else ifeq ($(findstring xcoreai, $(CONFIG)), xcoreai)
  TARGET = XCORE-AI-EXPLORER
endif

# The USED_MODULES variable lists other module used by the application.

USED_MODULES = lib_i2c lib_logging

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
