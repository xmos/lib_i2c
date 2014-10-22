import xmostest

class I2CMasterChecker(xmostest.SimThread):
    """"
    This simulator thread will act as I2C slave and check any transactions
    caused by the master.
    """

    def __init__(self, scl_port, sda_port, expected_speed,
                 tx_data = [], ack_sequence = []):
        self._scl_port = scl_port
        self._sda_port = sda_port
        self._tx_data = tx_data
        self._ack_sequence = ack_sequence
        self._expected_speed = expected_speed
        print "Checking I2C: SCL=%s, SDA=%s" % (self._scl_port, self._sda_port)

    def get_port_val(self, xsi, port):
        "Sample port, modelling the pull up"
        is_driving = xsi.is_port_driving(port)
        if not is_driving:
            return 1
        else:
            return xsi.sample_port_pins(port);

    def get_next_ack(self):
        if self._ack_index >= len(self._ack_sequence):
            return True
        else:
            ack = self._ack_sequence[self._ack_index]
            self._ack_index += 1
            return ack

    def read_byte(self, xsi):
       data = 0
       bit_num = 0
       received_stop = False
       received_start = False
       prev_fall_time = None
       bit_times = []
       while True:
           # Wait for clock to go high or for the xCORE to
           # stop driving
           xsi.wait_for_port_pins_change([self._scl_port])

           # Drive the clock port high (modelling the pull up)
           xsi.drive_port_pins(self._scl_port, 1)
           if bit_num == 8:
               print("Byte received: 0x%x" % data)
               ack = self.get_next_ack()
               if xsi.is_port_driving(self._sda_port):
                   print("WARNING: master driving SDA during ACK phase")
               if ack:
                   print("Sending ack")
                   xsi.drive_port_pins(self._sda_port, 0)
               else:
                   print("Sending nack")
                   xsi.drive_port_pins(self._sda_port, 1)
           else:
               bit = self.get_port_val(xsi, self._sda_port);
           xsi.wait_for_port_pins_change([self._scl_port, self._sda_port])
           sda_value = self.get_port_val(xsi, self._sda_port);

           if bit_num == 8 and self.get_port_val(xsi, self._scl_port) != 0:
               print("ERROR: clock pulse incomplete at end of byte")

           if bit_num == 8 and xsi.is_port_driving(self._sda_port):
               print("WARNING: master driving SDA during ACK phase")

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
           if (speed_in_kbps < 0.99 * self._expected_speed):
               print "ERROR: speed is <1% slower than expected"
     
           if (speed_in_kbps > self._expected_speed):
               print "ERROR: speed is faster than expected"

       if bit_num == 8:
            return False, False, data
       if bit_num != 0:
            print("ERROR: transaction finished during partial byte transfer")
     
       return received_stop, received_start, None

    def get_next_data_item(self):
        if self._tx_data_index >= len(self._tx_data):
            return 0xab
        else:
            data = self._tx_data[self._tx_data_index]
            self._tx_data_index += 1
            return data
     
    def write_bytes(self, xsi):
       data = self.get_next_data_item()
       bit_num = 0
       received_stop = False
       received_start = False
       prev_fall_time = None
       bit_times = []
       xsi.drive_port_pins(self._sda_port, (data & 0x80) >> 7)
       data <<= 1
       while True:
           # Wait for clock to go high or for the xCORE to
           # stop driving
           xsi.wait_for_port_pins_change([self._scl_port])
     
           # Drive the clock port high (modelling the pull up)
           xsi.drive_port_pins(self._scl_port, 1)
           if bit_num == 8:
               ack = self.get_port_val(xsi, self._sda_port)
               print("Master sends %s." % ("ACK" if ack==0 else "NACK"));
               if ack == 1:
                   break
               else:
                   data = self.get_next_data_item()
                   bit_num = -1
           else:
               if xsi.is_port_driving(self._sda_port):
                   print("ERROR: master driving SDA during slave data write")

           xsi.wait_for_port_pins_change([self._scl_port])
           fall_time = xsi.get_time()
     
           if prev_fall_time:
                bit_times.append(fall_time - prev_fall_time)
     
           prev_fall_time = fall_time
     
           bit_num += 1
           if bit_num == 8:
               print "Byte sent"
               # Turn the port around
               xsi.sample_port_pins(self._sda_port)
           else:
               xsi.drive_port_pins(self._sda_port, (data & 0x80) >> 7)
               data <<= 1
     
       if bit_times:
           avg_bit_time = sum(bit_times) / len(bit_times)
           speed_in_kbps = pow(10, 6) / avg_bit_time
           print "Speed = %d Kbps" % int(speed_in_kbps + .5)
           if (speed_in_kbps < 0.99 * self._expected_speed):
               print "ERROR: speed is <1% slower than expected"
     
           if (speed_in_kbps > self._expected_speed):
               print "ERROR: speed is faster than expected"
     
     
       print("Waiting for stop/start bit")
       # Wait for clock to go low then high again
       xsi.wait_for_port_pins_change([self._scl_port])
       xsi.wait_for_port_pins_change([self._scl_port])
       xsi.drive_port_pins(self._scl_port, 1)
       bit = self.get_port_val(xsi, self._sda_port)
       xsi.wait_for_port_pins_change([self._scl_port, self._sda_port])
       sda_value = self.get_port_val(xsi, self._sda_port);
       if self.get_port_val(xsi, self._scl_port) == 0:
           print("ERROR: SCL driven low before stop/repeated start")
           xsi.wait_for_port_pins_change([self._sda_port])
     
       if sda_value == 1:
           print("Stop bit received");
           return True, False
       if sda_value == 0:
           print("Repeated start bit received")
           return False, True
     
    def run(self, xsi):
        self._tx_data_index = 0
        self._ack_index = 0
        scl_value = self.get_port_val(xsi, self._scl_port);
        sda_value = self.get_port_val(xsi, self._sda_port);
        if (scl_value != 1 or sda_value != 1):
            print("ERROR: SDA or SCL not high at initialization")
        check_for_start_bit = True
        while True:
            if check_for_start_bit:
                xsi.wait_for_port_pins_change([self._scl_port, self._sda_port])
                scl_value = self.get_port_val(xsi, self._scl_port);
                if self.get_port_val(xsi, self._scl_port) != 1:
                    print("ERROR: SCL driven low before transaction started")
                else:
                    print("Start bit received")
     
            xsi.wait_for_port_pins_change([self._scl_port, self._sda_port])
            if self.get_port_val(xsi, self._scl_port) != 0:
                print("ERROR: SDA changed before clock driven low")
     
            got_stop, got_repeat_start, data = self.read_byte(xsi)
            if got_stop or got_repeat_start:
                print("ERROR:stop or repeated start bit during address tx")
                return
     
            mode = data & 1
            print("Master %s transaction started, device address=0x%x" %
                                   ("write" if mode == 0 else "read", data >> 1))
     
            if mode == 0:
              while True:
                got_stop, got_repeat_start, data = self.read_byte(xsi)

                if got_stop:
                    check_for_start_bit = True
                    break
                if got_repeat_start:
                    check_for_start_bit = False
                    break
            else:
              while True:
                got_stop, got_repeat_start = self.write_bytes(xsi)
                if got_stop:
                    check_for_start_bit = True
                    break
                if got_repeat_start:
                    check_for_start_bit = False
                    break
