ifeq ($(USING_XWAF),1)
all:
	xwaf configure build --config $(CONFIG)
else # USING_XWAF

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

PORT_SETUPS = 0 1 2 3 4
SPEEDS = 10 100 400
STOPS = stop no_stop

COMMON_FLAGS = -O2 -g -save-temps -DDEBUG_PRINT_ENABLE=1 -report

stop_val = 1
no_stop_val = 0

define create_flags
XCC_FLAGS_rx_tx_$(1)_$(2)_$(3) = $(COMMON_FLAGS) -DENABLE_TX=1 -DENABLE_RX=1 -DSPEED=$(1) -DSTOP=$$($(2)_val) -DPORT_SETUP=$(3)
endef

$(foreach speed,$(SPEEDS),\
  $(foreach stop,$(STOPS),\
	$(foreach port_setup,$(PORT_SETUPS),\
		$(eval $(call create_flags,$(speed),$(stop),$(port_setup))))))


XCC_FLAGS_tx_only_stop      = $(COMMON_FLAGS) -DENABLE_TX=1 -DENABLE_RX=0 -DSPEED=400 -DSTOP=1 -DPORT_SETUP=0
XCC_FLAGS_tx_only_no_stop   = $(COMMON_FLAGS) -DENABLE_TX=1 -DENABLE_RX=0 -DSPEED=400 -DSTOP=0 -DPORT_SETUP=0


TARGET = XCORE-AI-EXPLORER

# The USED_MODULES variable lists other module used by the application.

USED_MODULES = lib_i2c lib_logging

# Added for testing with Tools 15.0.2.  Without including dependency includes
# building with xmake fails.
INCLUDE_DIRS = ../../../lib_xassert/lib_xassert/api \
               ../../../lib_logging/lib_logging/api

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
endif # USING_XWAF
