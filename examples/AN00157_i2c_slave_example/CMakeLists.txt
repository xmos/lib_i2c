cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(AN00157_i2c_slave_example)

option(XCOREAI "Build for xcore.ai instead of xcore-200" ON)

if(XCOREAI)
    set(APP_HW_TARGET XCORE-AI-EXPLORER)
else()
    set(APP_HW_TARGET XCORE-200-EXPLORER)
endif()

set(APP_COMPILER_FLAGS -O2 -g -DDEBUG_PRINT_ENABLE=1 -report)

include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()
