cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

# Get JSON lists
file(READ ${CMAKE_CURRENT_LIST_DIR}/test_params.json params_json)

# Get individual fields from params_json
string(JSON speed_list GET ${params_json} SPEEDS)
string(JSON stop_list GET ${params_json} STOPS)
string(JSON comb_list GET ${params_json} COMBS)

string(JSON speed_list_len LENGTH ${speed_list})
string(JSON stop_list_len LENGTH ${stop_list})
string(JSON comb_list_len LENGTH ${comb_list})

# Subtract one off each of the lengths because RANGE includes last element

math(EXPR speed_list_len "${speed_list_len} - 1")
math(EXPR stop_list_len "${stop_list_len} - 1")
math(EXPR comb_list_len "${comb_list_len} - 1")


set(APP_PCA_ENABLE ON)
set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/../../..)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

set(HW_TARGETS xs2 xcoreai)

set(comb_val 1)
set(non_comb_val 0)
set(stop_val 1)
set(no_stop_val 0)

foreach(hwtarget ${HW_TARGETS})
    if(hwtarget STREQUAL "xcoreai")
        set(target "XCORE-AI-EXPLORER")
    elseif(hwtarget STREQUAL "xs2")
        set(target "XCORE-200-EXPLORER")
    endif()

    foreach(i RANGE 0 ${comb_list_len})
        string(JSON comb GET ${comb_list} ${i})

        foreach(j RANGE 0 ${speed_list_len})
            string(JSON speed GET ${speed_list} ${j})

            foreach(k RANGE 0 ${stop_list_len})
                string(JSON stop GET ${stop_list} ${k})
                set(config ${comb}_${speed}_${hwtarget}_${stop})
                message(STATUS "building config ${config} => ${${comb}_val}_${speed}_${hwtarget}_${${stop}_val}")

                project(i2c_master_async_test)
                set(APP_HW_TARGET   ${target})

                set(APP_COMPILER_FLAGS_${config}
                    -O2
                    -g
                    -DDEBUG_PRINT_ENABLE=1
                    -report
                    -DSPEED=${speed}
                    -DCOMB=${${comb}_val}
                    -DSTOP=${${stop}_val})

                XMOS_REGISTER_APP()

                unset(APP_COMPILER_FLAGS_${config})

            endforeach() # stop
        endforeach() # speed
    endforeach() # comb

    foreach(k RANGE 0 ${stop_list_len})
        string(JSON stop GET ${stop_list} ${k})
        set(config interfere_${hwtarget}_${stop})
        message(STATUS "building config ${config} => ${hwtarget}_${${stop}_val}")

        project(i2c_master_async_test)
        set(APP_HW_TARGET   ${target})

        set(APP_COMPILER_FLAGS_${config}
            -O2
            -g
            -DDEBUG_PRINT_ENABLE=1
            -report
            -DSPEED=100
            -DCOMB=1
            -DSTOP=${${stop}_val}
            -DINTERFERE)

        XMOS_REGISTER_APP()
        unset(APP_COMPILER_FLAGS_${config})

    endforeach() # stop
endforeach() # hwtarget




