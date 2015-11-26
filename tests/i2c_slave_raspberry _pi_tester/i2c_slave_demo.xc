// Copyright (c) 2015, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <platform.h>
#include <stdio.h>
#include "i2c.h"
#include <xscope.h>
#include <print.h>
#include "debug_print.h"

//on tile[0]: port p_scl = XS1_PORT_1E;
//on tile[0]: port p_sda = XS1_PORT_1F;
on tile[0]: port p_scl_s = XS1_PORT_1F;
on tile[0]: port p_sda_s = XS1_PORT_1H;

#define DEV_ADDR 0x4f

void demo(client interface i2c_master_if i2c)
{
  i2c_regop_res_t result;
  result = i2c.write_reg(DEV_ADDR, 0x07, 0x12);
  if (result != I2C_REGOP_SUCCESS) {
    debug_printf("Write reg failed!\n");
  }

  result = i2c.write_reg(DEV_ADDR, 0x08, 0x78);
  if (result != I2C_REGOP_SUCCESS) {
    debug_printf("Write reg failed!\n");
  }

  for (int i=0;i<16;i++)
  {
  unsigned char data = i2c.read_reg(DEV_ADDR, 0x07, result);
  if (result != I2C_REGOP_SUCCESS) {
    debug_printf("Read reg failed!\n");
  }
  debug_printf("Read data %x\n",data);
  }

}
void i2c_slave_register_file(server i2c_slave_callback_if i2c)
{
    uint8_t outData[256];
    for (int i=0;i<256;i++) outData[i]=i;
    int new_reg=1,regnum;
    while (1) {
        select {
        case i2c.start_read_request(void):
            break;
        case i2c.ack_read_request(void) -> i2c_slave_ack_t response:
            response = I2C_SLAVE_ACK;
         //   debug_printf("RD:ACK\n");
            break;
        case i2c.start_write_request(void):
            new_reg = 1;
            break;
        case i2c.ack_write_request(void) -> i2c_slave_ack_t response:
            response = I2C_SLAVE_ACK;
            break;
        case i2c.start_master_write(void):

            // Prepare to receive data
            break;
        case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
               if (new_reg) { regnum = data; new_reg = 0;}
               else {
//               printhexln(regnum);
//               printhexln(data);
               outData[regnum] = data;
               regnum++;
               }
             // Handle received data
//                printstr("WR Data: ");
//                printhexln(data);
            response = I2C_SLAVE_ACK;
            break;
        case i2c.start_master_read(void):
            break;
        case i2c.master_requires_data() -> uint8_t data:
            data = outData[regnum];
            regnum++;
          //  outData++;
            break;
        case i2c.stop_bit():
            new_reg = 1;
            break;
        }
    }
}

int main()
{
    i2c_master_if i2c[1];
    i2c_slave_callback_if i2c_s;



    par
    {
    /*    on tile[0]:
        {
           par
           {
               i2c_master(i2c, 1, p_scl, p_sda, 10);
               demo(i2c[0]);
            }
        } */
        on tile[0]:
        {
            par
            {
           i2c_slave(i2c_s,p_scl_s,p_sda_s,DEV_ADDR);
           i2c_slave_register_file(i2c_s);
            }
        }

    }
    return 0;
}

