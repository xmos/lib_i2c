cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

# Get JSON lists
file(READ ${CMAKE_CURRENT_LIST_DIR}/test_params.json params_json)

# Get individual fields from params_json
string(JSON speed_list GET ${params_json} SPEEDS)
string(JSON stop_list GET ${params_json} STOPS)

string(JSON speed_list_len LENGTH ${speed_list})
string(JSON stop_list_len LENGTH ${stop_list})

# Subtract one off each of the lengths because RANGE includes last element

math(EXPR speed_list_len "${speed_list_len} - 1")
math(EXPR stop_list_len "${stop_list_len} - 1")

set(APP_PCA_ENABLE ON)
set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

set(ARCH xs2 xs3)

set(stop_val 1)
set(no_stop_val 0)

foreach(arch ${ARCH})
    if(arch STREQUAL "xs3")
        set(target "XK-EVK-XU316")
    elseif(arch STREQUAL "xs2")
        set(target "XCORE-200-EXPLORER")
    endif()
    foreach(j RANGE 0 ${speed_list_len})
        string(JSON speed GET ${speed_list} ${j})

        foreach(k RANGE 0 ${stop_list_len})
            string(JSON stop GET ${stop_list} ${k})
            set(config ${speed}_${stop}_${arch})
            message(STATUS "building config ${speed}_${stop}_${arch} => ${speed}_${${stop}_val}_${arch}")

            project(i2c_sp_test)
            set(APP_HW_TARGET   ${target})

            set(APP_COMPILER_FLAGS_${config}
                -O2
                -g
                -DDEBUG_PRINT_ENABLE=1
                -report
                -DSPEED=${speed}
                -DSTOP=${${stop}_val})

            XMOS_REGISTER_APP()
            unset(APP_COMPILER_FLAGS_${config})

        endforeach()
    endforeach()
endforeach()
