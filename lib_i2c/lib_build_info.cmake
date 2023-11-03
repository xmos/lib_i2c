set(LIB_NAME lib_i2c)
set(LIB_VERSION 6.1.1)
set(LIB_INCLUDES api)
set(LIB_DEPENDENT_MODULES "lib_xassert"
                          "lib_logging")
set(LIB_COMPILER_FLAGS -Os)

XMOS_REGISTER_MODULE()
