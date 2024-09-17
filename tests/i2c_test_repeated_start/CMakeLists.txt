cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

project(i2c_test_repeated_start)

set(APP_PCA_ENABLE ON)
set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(APP_HW_TARGET   XCORE-AI-EXPLORER)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

set(APP_COMPILER_FLAGS
                -O2
                -g
                -DDEBUG_PRINT_ENABLE=1
                -report)

XMOS_REGISTER_APP()
