set(LIB_NAME lib_i2c)
set(LIB_VERSION 6.4.0)
set(LIB_INCLUDES api)
set(LIB_DEPENDENT_MODULES "lib_xassert(4.3.1)")
set(LIB_COMPILER_FLAGS  -Os
                        -Wall
                        -Wextra
                        -Wsign-compare
                        -Wconversion)

XMOS_REGISTER_MODULE()
