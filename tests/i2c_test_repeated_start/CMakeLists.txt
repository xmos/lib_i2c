cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

set(APP_PCA_ENABLE ON)
set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

set(HW_TARGETS xs2 xcoreai)

foreach(hwtarget ${HW_TARGETS})
    if(hwtarget STREQUAL "xcoreai")
        set(target "XCORE-AI-EXPLORER")
    elseif(hwtarget STREQUAL "xs2")
        set(target "XCORE-200-EXPLORER")
    endif()

    project(i2c_test_repeated_start)
    set(APP_HW_TARGET   ${target})

    set(APP_COMPILER_FLAGS_${hwtarget}
                    -O2
                    -g
                    -DDEBUG_PRINT_ENABLE=1
                    -report)

    XMOS_REGISTER_APP()
    unset(APP_COMPILER_FLAGS_${hwtarget})
endforeach()
