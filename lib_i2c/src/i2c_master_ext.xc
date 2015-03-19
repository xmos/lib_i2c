// Copyright (c) 2015, XMOS Ltd, All rights reserved
#include <i2c.h>

/* This file provides external definitions for the inline functions declared in
   i2c.h (using C99 inlining semantics) */

extends client interface i2c_master_if : {
  extern inline uint8_t read_reg_8_8(client interface i2c_master_if i,
                                     uint8_t device_addr, uint8_t reg,
                                     i2c_regop_res_t &res);

  extern inline uint8_t read_reg(client interface i2c_master_if i,
                                 uint8_t device_addr, uint8_t reg,
                                 i2c_regop_res_t &res);

  extern inline i2c_regop_res_t write_reg_n_m(client interface i2c_master_if i,
                                   uint8_t device_addr,
                                   uint8_t reg[m],
                                   size_t m,
                                   uint8_t data[n],
                                   size_t n);

  extern inline i2c_regop_res_t write_reg_8_8(client interface i2c_master_if i,
                                   uint8_t device_addr, uint8_t reg,
                                   uint8_t data);

  extern inline i2c_regop_res_t write_reg(client interface i2c_master_if i,
                                    uint8_t device_addr,
                                    uint8_t reg, uint8_t data);

  extern inline uint8_t read_reg8_addr16(client interface i2c_master_if i,
                                         uint8_t device_addr, uint16_t reg,
                                         i2c_regop_res_t &res);

  extern inline i2c_regop_res_t write_reg8_addr16(client interface i2c_master_if i,
                                       uint8_t device_addr, uint16_t reg,
                                       uint8_t data);

  extern inline uint16_t read_reg16(client interface i2c_master_if i,
                                    uint8_t device_addr, uint16_t reg,
                                    i2c_regop_res_t &res);

 extern inline i2c_regop_res_t write_reg16(client interface i2c_master_if i,
                                uint8_t device_addr, uint16_t reg,
                                uint16_t data);

  extern inline i2c_regop_res_t write_reg16_addr8(client interface i2c_master_if i,
                                                  uint8_t device_addr, uint8_t reg,
                                                  uint16_t data);

  extern inline uint16_t read_reg16_addr8(client interface i2c_master_if i,
                                          uint8_t device_addr, uint8_t reg,
                                          i2c_regop_res_t &result);


}
