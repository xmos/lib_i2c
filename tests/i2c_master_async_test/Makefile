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

ARCHS = xs1 xs2
SPEEDS = 10 100 400
STOPS = stop no_stop
COMBS = comb non_comb

COMMON_FLAGS = -O2 -g -save-temps -DDEBUG_PRINT_ENABLE=1 -report

stop_val = 1
no_stop_val = 0

comb_val = 1
non_comb_val = 0

define create_flags
XCC_FLAGS_$(1)_$(2)_$(3)_$(4) = $(COMMON_FLAGS) -DSPEED=$(2) -DCOMB=$$($(1)_val) -DSTOP=$$($(4)_val)
endef

$(foreach arch,$(ARCHS),\
  $(foreach speed,$(SPEEDS),\
    $(foreach stop,$(STOPS),\
      $(foreach comb,$(COMBS),\
        $(eval $(call create_flags,$(comb),$(speed),$(arch),$(stop)))))))

define create_flags_interfere
XCC_FLAGS_interfere_$(1)_$(2) = $(COMMON_FLAGS) -DSPEED=100 -DCOMB=1 -DSTOP=$$($(2)_val) -DINTERFERE
endef

$(foreach arch,$(ARCHS),\
  $(foreach stop,$(STOPS),\
    $(eval $(call create_flags_interfere,$(arch),$(stop)))))

ifeq ($(findstring xs1, $(CONFIG)), xs1)
  TARGET = SLICEKIT-L16
else
  TARGET = XCORE-200-EXPLORER
endif

# The USED_MODULES variable lists other module used by the application.

USED_MODULES = lib_i2c lib_logging

#=============================================================================
# The following part of the Makefile includes the common build infrastructure
# for compiling XMOS applications. You should not need to edit below here.

XMOS_MAKE_PATH ?= ../..
include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
endif # USING_XWAF
