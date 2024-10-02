cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_i2c_tests)

add_subdirectory(i2c_master_async_test)
add_subdirectory(i2c_master_reg_test)
add_subdirectory(i2c_master_test)
add_subdirectory(i2c_slave_test)
add_subdirectory(i2c_sp_test)
add_subdirectory(i2c_test_locks)
add_subdirectory(i2c_test_repeated_start)

