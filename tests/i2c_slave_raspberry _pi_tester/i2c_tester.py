#!/usr/bin/python

import sys
from I2C import I2C

def write1():
 reg =  0x00
 value = 70
 while (reg <= 255):
   bus.write8(reg,value)
   reg = reg+1

def write_read():
 reg =  0x00
 value = 50
 while (reg <= 255):
   bus.write8(reg,value)
   result = bus.readU8(reg)
   if value == result:
      print 'Read byte success'
   else:
     print 'Read data value did not match written value'
     print 'register address:%d' % reg
     print 'Value read: %d' % result
     print 'Correct Value: %d' % value
     bus.errMsg()
   reg = reg+1

def read1():
 reg =  0x00
 value = 70
 while (reg <= 255):
   result = bus.readU8(reg)
   if value == result:
      print 'Read byte success'
   else:
     print 'Read data value did not match written value'
     print 'register address:%d' % reg
     print 'Value read: %d' % result
     print 'Correct Value: %d' %value
     bus.errMsg()
   reg = reg+1  

def write_word1():
  reg =  0x00
  value = 0x15c2
  while (reg <= 255):
    bus.write16(reg,value)
    reg = reg+2
    
def read_word1():
 reg =  0x00
 value = 0x15c2
 while (reg <= 255):
   result = bus.readU16(reg)
   if value == result:
      print 'Read word success'
   else:
     print 'Read data value did not match written value'
     print 'register address:%d' % reg
     print 'Value read: %d' % result
     print 'Correct Value: %d' %value
     bus.errMsg()
   reg = reg+2

def write_read_word():
  reg =  0x00
  value = 0x15c2
  while (reg <= 255):
    bus.write16(reg,value)
    result = bus.readU16(reg)
    if value == result:
       print 'Read word success'
    else:
      print 'Read data value did not match written value'
      print 'register address:%d' % reg
      print 'Value read: %d' % result
      print 'Correct Value: %d' %value
      bus.errMsg()
    reg = reg+2

if __name__== '__main__':
 address = 0x4f
 bus = I2C(address)
 write1()
 read1()
 write_read()
 write_word1()
 read_word1()
 write_read_word()

 
 

