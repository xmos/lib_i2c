set(LIB_NAME lib_i2c)
set(LIB_VERSION 6.2.0)
set(LIB_INCLUDES api)
set(LIB_DEPENDENT_MODULES lib_xassert(4.2.0)
                          lib_logging(3.2.0))
set(LIB_COMPILER_FLAGS -Os)

XMOS_REGISTER_MODULE()
