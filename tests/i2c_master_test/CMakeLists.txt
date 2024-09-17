cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

# Get JSON lists
file(READ ${CMAKE_CURRENT_LIST_DIR}/test_params.json params_json)

# Get individual fields from params_json
string(JSON speed_list GET ${params_json} SPEEDS)
string(JSON stop_list GET ${params_json} STOPS)
string(JSON dir_list GET ${params_json} DIRECTION)

string(JSON speed_list_len LENGTH ${speed_list})
string(JSON stop_list_len LENGTH ${stop_list})
string(JSON dir_list_len LENGTH ${dir_list})

# Subtract one off each of the lengths because RANGE includes last element

math(EXPR speed_list_len "${speed_list_len} - 1")
math(EXPR stop_list_len "${stop_list_len} - 1")
math(EXPR dir_list_len "${dir_list_len} - 1")

project(i2c_master_test)

set(APP_PCA_ENABLE ON)
set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(APP_HW_TARGET   XCORE-AI-EXPLORER)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

if(APP_HW_TARGET STREQUAL "XCORE-AI-EXPLORER")
    set(target "xcoreai")
elseif(APP_HW_TARGET STREQUAL "XCORE-200-EXPLORER")
    set(target "xs2")
endif()

set(stop_val 1)
set(no_stop_val 0)

foreach(i RANGE 0 ${dir_list_len})
    string(JSON dir GET ${dir_list} ${i})

    foreach(j RANGE 0 ${speed_list_len})
        string(JSON speed GET ${speed_list} ${j})

        foreach(k RANGE 0 ${stop_list_len})
            string(JSON stop GET ${stop_list} ${k})
            set(config ${dir}_${speed}_${stop}_${target})
            message(STATUS "building config ${config} => ${dir}_${speed}_${${stop}_val}_${target}")

            if(dir STREQUAL "rx_tx")
                set(rx_enable 1)
                set(tx_enable 1)
            elseif(dir STREQUAL "tx_only")
                set(rx_enable 0)
                set(tx_enable 1)
            else()
                set(rx_enable 1)
                set(tx_enable 0)
            endif()

            set(APP_COMPILER_FLAGS_${config}
                -O2
                -g
                -DDEBUG_PRINT_ENABLE=1
                -report
                -DSPEED=${speed}
                -DENABLE_TX=${tx_enable}
                -DENABLE_RX=${rx_enable}
                -DSTOP=${${stop}_val})

        endforeach()
    endforeach()
endforeach()

XMOS_REGISTER_APP()
