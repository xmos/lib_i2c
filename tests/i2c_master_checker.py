import xmostest

def get_port_val(xsi, port):
    "Sample port, modelling the pull up"
    if not xsi.is_port_driving(port):
        return 1
    else:
        return xsi.sample_port_pins(port);

def read_byte(xsi, scl_port, sda_port, expected_speed):
   data = 0
   bit_num = 0
   received_stop = False
   received_start = False
   prev_fall_time = None
   bit_times = []
   while True:
       # Wait for clock to go high or for the xCORE to
       # stop driving
       xsi.wait_for_port_pins_change([scl_port])

       # Drive the clock port high (modelling the pull up)
       xsi.drive_port_pins(scl_port, 1)
       if bit_num == 8:
           print("Byte received: 0x%x" % data)
           print("Sending ack")
           xsi.drive_port_pins(sda_port, 1)
       else:
           bit = get_port_val(xsi, sda_port);
       xsi.wait_for_port_pins_change([scl_port, sda_port])
       sda_value = get_port_val(xsi, sda_port);

       if bit_num == 8 and get_port_val(xsi, scl_port) != 0:
           print("ERROR: clock pulse incomplete at end of byte")

       if bit_num == 8:
           break

       if bit_num != 8 and sda_value != bit and sda_value == 1:
           print("Stop bit received");
           received_stop = True
           break
       if bit_num != 8 and sda_value != bit and sda_value == 0:
           print("Repeated start bit received")
           received_start = True
           break

       fall_time = xsi.get_time()

       if prev_fall_time:
            bit_times.append(fall_time - prev_fall_time)

       prev_fall_time = fall_time


       data = (data << 1) | bit;
       bit_num += 1

   if bit_times:
       avg_bit_time = sum(bit_times) / len(bit_times)
       speed_in_kbps = pow(10, 6) / avg_bit_time
       print "Speed = %d Kbps" % int(speed_in_kbps + .5)
       if (speed_in_kbps < 0.99 * expected_speed):
           print "ERROR: speed is <1% slower than expected"

       if (speed_in_kbps > expected_speed):
           print "ERROR: speed is faster than expected"



   if bit_num == 8:
        return False, False, data
   if bit_num != 0:
        print("ERROR: transaction finished during partial byte transfer")

   return received_stop, received_start, None


def write_bytes(xsi, scl_port, sda_port, tx_data, expected_speed):
   data = tx_data.pop() if tx_data else 0xab
   bit_num = 0
   received_stop = False
   received_start = False
   prev_fall_time = None
   bit_times = []
   xsi.drive_port_pins(sda_port, (data & 0x80) >> 7)
   data <<= 1
   while True:
       # Wait for clock to go high or for the xCORE to
       # stop driving
       xsi.wait_for_port_pins_change([scl_port])

       # Drive the clock port high (modelling the pull up)
       xsi.drive_port_pins(scl_port, 1)
       if bit_num == 8:
           ack = get_port_val(xsi, sda_port)
           print("Master sends %s." % ("ACK" if ack==0 else "NACK"));
           if ack == 1:
               break
           else:
               data = tx_data.pop() if tx_data else 0xab
               bit_num = -1
       xsi.wait_for_port_pins_change([scl_port])
       fall_time = xsi.get_time()

       if prev_fall_time:
            bit_times.append(fall_time - prev_fall_time)

       prev_fall_time = fall_time

       bit_num += 1
       if bit_num == 8:
           print "Byte sent"
           # Turn the port around
           xsi.sample_port_pins(sda_port)
       else:
           xsi.drive_port_pins(sda_port, (data & 0x80) >> 7)
           data <<= 1

   if bit_times:
       avg_bit_time = sum(bit_times) / len(bit_times)
       speed_in_kbps = pow(10, 6) / avg_bit_time
       print "Speed = %d Kbps" % int(speed_in_kbps + .5)
       if (speed_in_kbps < 0.99 * expected_speed):
           print "ERROR: speed is <1% slower than expected"

       if (speed_in_kbps > expected_speed):
           print "ERROR: speed is faster than expected"


   print("Waiting for stop/start bit")
   # Wait for clock to go low then high again
   xsi.wait_for_port_pins_change([scl_port])
   xsi.wait_for_port_pins_change([scl_port])
   xsi.drive_port_pins(scl_port, 1)
   bit = get_port_val(xsi, sda_port)
   xsi.wait_for_port_pins_change([scl_port, sda_port])
   sda_value = get_port_val(xsi, sda_port);
   if get_port_val(xsi, scl_port) == 0:
       print("ERROR: SCL driven low before stop/repeated start")
       xsi.wait_for_port_pins_change([sda_port])

   if sda_value == 1:
       print("Stop bit received");
       return True, False
   if sda_value == 0:
       print("Repeated start bit received")
       return False, True

def i2c_master_checker(xsi, scl_port, sda_port, tx_data, expected_speed):
    """"
    This simulator thread will act as I2C slave and check any transactions
    caused by the master.
    """
    tx_data.reverse()
    scl_value = get_port_val(xsi, scl_port);
    sda_value = get_port_val(xsi, sda_port);
    if (scl_value != 1 or sda_value != 1):
        print("ERROR: SDA or SCL not high at initialization")
    check_for_start_bit = True
    while True:
        if check_for_start_bit:
            xsi.wait_for_port_pins_change([scl_port, sda_port])
            scl_value = get_port_val(xsi, scl_port);
            if get_port_val(xsi, scl_port) != 1:
                print("ERROR: SCL driven low before transaction started")
            else:
                print("Start bit received")

        xsi.wait_for_port_pins_change([scl_port, sda_port])
        if get_port_val(xsi, scl_port) != 0:
            print("ERROR: SDA changed before clock driven low")

        got_stop, got_repeat_start, data = \
           read_byte(xsi, scl_port, sda_port, expected_speed)
        if got_stop or got_repeat_start:
            print("ERROR:stop or repeated start bit during address tx")
            return

        mode = data & 1
        print("Master %s transaction started, device address=0x%x" %
                               ("write" if mode == 0 else "read", data >> 1))

        if mode == 0:
          while True:
            got_stop, got_repeat_start, data = \
                 read_byte(xsi, scl_port, sda_port, expected_speed)
            if got_stop:
                check_for_start_bit = True
                break
            if got_repeat_start:
                check_for_start_bit = False
                break
        else:
          while True:
            got_stop, got_repeat_start = \
              write_bytes(xsi, scl_port, sda_port, tx_data, expected_speed)
            if got_stop:
                check_for_start_bit = True
                break
            if got_repeat_start:
                check_for_start_bit = False
                break
