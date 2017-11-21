import xmostest

class I2CMasterChecker(xmostest.SimThread):
    """"
    This simulator thread will act as I2C slave and check any transactions
    caused by the master.
    """

    def __init__(self, scl_port, sda_port, expected_speed,
                 tx_data=[], ack_sequence=[], clock_stretch=0):
        self._scl_port = scl_port
        self._sda_port = sda_port
        self._tx_data = tx_data
        self._ack_sequence = ack_sequence
        self._expected_speed = expected_speed
        self._clock_stretch = clock_stretch

        self._external_scl_value = 0
        self._external_sda_value = 0

        self._scl_change_time = None
        self._sda_change_time = None

        self._bit_num = 0
        self._bit_times = []
        self._prev_fall_time = None
        self._byte_num = 0

        self._read_data = None
        self._write_data = None

        self._drive_ack = 1

        print "Checking I2C: SCL=%s, SDA=%s" % (self._scl_port, self._sda_port)

    def error(self, str):
         print "ERROR: %s (@ %s)" % (str, self.xsi.get_time())

    def read_port(self, port, external_value):
      driving = self.xsi.is_port_driving(port)
      if driving:
        value = self.xsi.sample_port_pins(port)
      else:
        value = external_value
        # Maintain the weak external drive
        self.xsi.drive_port_pins(port, external_value)
      # print "READ {}, got {}, {} ({}) @ {}".format(
      #   port, driving, value, external_value, self.xsi.get_time())
      return value

    def read_scl_value(self):
      return self.read_port(self._scl_port, self._external_scl_value)

    def read_sda_value(self):
      return self.read_port(self._sda_port, self._external_sda_value)

    def drive_scl(self, value):
       # Cache the value that is currently being driven
       self._external_scl_value = value
       self.xsi.drive_port_pins(self._scl_port, value)

    def drive_sda(self, value):
       # Cache the value that is currently being driven
       self._external_sda_value = value
       self.xsi.drive_port_pins(self._sda_port, value)

    def get_next_ack(self):
        if self._ack_index >= len(self._ack_sequence):
            return True
        else:
            ack = self._ack_sequence[self._ack_index]
            self._ack_index += 1
            return ack

    def check_data_valid_time(self, time):
        if time < 0:
            # Data change must have been for a previous bit
            return

        if (self._expected_speed == 100 and time > 3450) or\
           (self._expected_speed == 400 and time >  900):
            self.error("Data valid time not respected: %gns" % time)

    def check_low_time(self, time):
        if (self._expected_speed == 100 and time < 4700) or\
           (self._expected_speed == 400 and time < 1300):
            self.error("Clock low time less than minimum in spec: %gns" % time)

    def check_high_time(self, time):
        if (self._expected_speed == 100 and time < 4000) or\
           (self._expected_speed == 400 and time < 900):
            self.error("Clock high time less than minimum in spec: %gns" % time)

    def check_setup_stop_time(self, time):
        if (self._expected_speed == 100 and time < 4000) or\
           (self._expected_speed == 400 and time < 600):
            self.error("Stop bit setup time less than minimum in spec: %gns" % time)

    def get_next_data_item(self):
        if self._tx_data_index >= len(self._tx_data):
            return 0xab
        else:
            data = self._tx_data[self._tx_data_index]
            self._tx_data_index += 1
            return data

    states = {
      #                      EXPECT     |  Next state on transition of
      # STATE            :   SCL,  SDA  |  SCL       SDA
      #

      "STOPPED"          : ( 1,    1,     "ILLEGAL",          "STARTING" ),
      "STARTING"         : ( 1,    0,     "DRIVE_BIT",        "ILLEGAL" ),
      "DRIVE_BIT"        : ( 0,    None,  "SAMPLE_BIT",       "DRIVE_BIT" ),
      "SAMPLE_BIT"       : ( 1,    None,  "DRIVE_BIT",        "STOPPED" ),
      "BYTE_DONE"        : ( None, None,  "DRIVE_ACK",        "ILLEGAL" ),
      "DRIVE_ACK"        : ( 0,    None,  "SAMPLE_ACK",       "DRIVE_ACK" ),
      "ACK_SENT"         : ( 0,    None,  "SAMPLE_ACK",       "ACK_SENT" ),
      # This state will be transitioned by the handler
      "SAMPLE_ACK"       : ( 1,    None,  "NOT_POSSIBLE",     "NOT_POSSIBLE" ),
      "ACKED"            : ( None, None,  "DRIVE_BIT",        "ACKED" ),
      "NACKED"           : ( None, None,  "NACKED_SELECT",    "ILLEGAL" ),
      "NACKED_SELECT"    : ( 0,    1,     "GO_TO_REPEAT",     "GO_TO_STOPPED0" ),
      "GO_TO_STOPPED0"   : ( 0,    0,     "GO_TO_STOPPED1",   "ILLEGAL" ),
      "GO_TO_STOPPED1"   : ( 1,    0,     "ILLEGAL",          "STOPPED" ),
      "GO_TO_REPEAT"     : ( 1,    1,     "ILLEGAL",          "STOPPED" ),
      "REPEAT_START"     : ( 0,    1,     "ILLEGAL",          "STARTING" ),
      "ILLEGAL"          : ( None, None,  "ILLEGAL",          "ILLEGAL" ),
    }

    @property
    def expected_scl(self):
      return self.states[self._state][0]

    @property
    def expected_sda(self):
      return self.states[self._state][1]

    def wait_for_change(self):
      """ Wait for either the SDA/SCL port to change and return which one it was.
          Need to also maintain the drive of any value set by the user.
      """
      scl_changed = False
      sda_changed = False

      scl_value = self.read_scl_value()
      sda_value = self.read_sda_value()

      while True:
        self.wait_for_port_pins_change([self._scl_port, self._sda_port])
        new_scl_value = self.read_scl_value()
        new_sda_value = self.read_sda_value()

        # When a multi-bit port is being used it is essential to wait until
        # these bits actually change
        if new_scl_value != scl_value or new_sda_value != sda_value:
          break

      time_now = self.xsi.get_time()

      print "wait_for_change {},{} -> {},{} @ {}".format(
        scl_value, sda_value, new_scl_value, new_sda_value, time_now)

      #
      # SCL changed
      #
      if scl_value != new_scl_value:
        scl_changed = True

        if self._scl_change_time:
          if new_scl_value == 0:
            self.check_high_time(time_now - self._scl_change_time)
          else:
            self.check_low_time(time_now - self._scl_change_time)

        self._scl_change_time = time_now

        # Record the time of the falling edges
        if new_scl_value == 0:
          fall_time = self.xsi.get_time()
          if self._prev_fall_time is not None:
            self._bit_times.append(fall_time - self._prev_fall_time)
          self._prev_fall_time = fall_time

      #
      # SDA changed
      #
      if sda_value != new_sda_value:
        sda_changed = True
        self._sda_change_time = time_now

        if new_scl_value == 0:
          self.check_data_valid_time(time_now - self._scl_change_time)

        if self._state == "GO_TO_STOPPED1":
          self.check_setup_stop_time(time_now - self._scl_change_time)

      return scl_changed, sda_changed

    def set_state(self, next_state):
      print "State: {} -> {} @ {}".format(self._state, next_state, self.xsi.get_time())
      self._prev_state = self._state
      self._state = next_state

    def move_to_next_state(self, scl_changed, sda_changed):
      if scl_changed:
        next_state = self.states[self._state][2]
      else:
        next_state = self.states[self._state][3]
      self.set_state(next_state)

    def check_value(self, value, expected, name):
      if expected is not None:
        if value != expected:
          self.error("{}: {} != {}".format(self._state, name, expected))

    def check_scl_sda_lines(self):
      self.check_value(self.read_scl_value(), self.expected_scl, "SCL")
      self.check_value(self.read_sda_value(), self.expected_sda, "SDA")

    def start_read(self):
      self._bit_num = 0
      self._bit_times = []
      self._prev_fall_time = None
      self._read_data = 0
      self._write_data = None

    def start_write(self):
      self._bit_num = 0
      self._bit_times = []
      self._prev_fall_time = None
      self._read_data = None
      self._write_data = self.get_next_data_item()

    def byte_done(self):
      if self._read_data is not None:
        print("Byte received: 0x%x" % self._read_data)
        self._drive_ack = 1
      else:
        # Reads are acked by the master
        self._drive_ack = 0
        print("Byte sent")

      if self._bit_times:
        avg_bit_time = sum(self._bit_times) / len(self._bit_times)
        speed_in_kbps = pow(10, 6) / avg_bit_time
        print "Speed = %d Kbps" % int(speed_in_kbps + .5)
        if self._expected_speed != None and \
          (speed_in_kbps < 0.99 * self._expected_speed):
           print "ERROR: speed is <1% slower than expected"

        if self._expected_speed != None and \
          (speed_in_kbps > self._expected_speed * 1.05):
           print "ERROR: speed is faster than expected"

      if self._byte_num == 0:
        # Command byte

        # The command is always acked by the slave
        self._drive_ack = 1

        # Determine whether it is starting a read or write
        mode = self._read_data & 0x1
        print("Master %s transaction started, device address=0x%x" %
              ("write" if mode == 0 else "read", self._read_data >> 1))

        if mode == 0:
          self.start_read()
        else:
          self.start_write()

      else:
        if self._read_data is not None:
          self.start_read()

      self.set_state("BYTE_DONE")

    def handle_nacked_select(self):
      pass

    def handle_go_to_stopped0(self):
      pass

    def handle_go_to_stopped1(self):
      print("Stop bit received");

    def handle_stopped(self):
      pass

    def handle_starting(self):
      print("Start bit received")
      self._byte_num = 0
      self.start_read()

    def handle_illegal(self):
      self.error("Illegal state arrived at from {}".format(self._prev_state))

    def handle_drive_bit(self):
      if self._write_data is not None:
        self.drive_sda((self._write_data & 0x80) >> 7)

    def handle_sample_bit(self):
      if self._read_data is not None:
        self._read_data = (self._read_data << 1) | self.read_sda_value()

      if self._write_data is not None:
        self._write_data = (self._write_data << 1) & 0xff

      self._bit_num += 1
      if self._bit_num == 8:
        self.byte_done()

    def handle_drive_ack(self):
      if self._drive_ack:
        ack = self.get_next_ack()
        if ack:
          print("Sending ack")
          self.drive_sda(0)
        else:
          print("Sending nack")
          self.drive_sda(1)
        self.set_state("ACK_SENT")
      else:
        # Pull up so that the master can release the bus
        self.drive_sda(1)

    def handle_ack_sent(self):
      pass

    def handle_sample_ack(self):
      if self._drive_ack:
        if self.xsi.is_port_driving(self._sda_port):
          print("WARNING: master driving SDA during ACK phase")

        if self.read_sda_value():
          self.set_state("NACKED")
        else:
          self.set_state("ACKED")

      else:
        nack = self.read_sda_value()
        print("Master sends %s." % ("NACK" if nack else "ACK"));
        if nack:
          self.set_state("NACKED")
          print("Waiting for stop/start bit")
        else:
          self.set_state("ACKED")
          # Prepare the next byte to be read
          self._write_data = self.get_next_data_item()

      self._bit_num = 0
      self._byte_num += 1

    def handle_acked(self):
      pass

    def handle_repeat_start(self):
      print("Repeated start bit received")

    def run(self):
      # Simulate external pullup
      self.drive_scl(1)
      self.drive_sda(1)

      # Ignore the blips on the ports at the start
      self.wait_until(10000)

      self._tx_data_index = 0
      self._ack_index = 0

      self._state = "STOPPED"

      while True:
        self.check_scl_sda_lines()

        handler = getattr(self, "handle_" + self._state.lower())
        handler()

        scl_changed, sda_changed = self.wait_for_change()

        if scl_changed and sda_changed:
          self.error("SCL & SDA changed simultaneously")

        self.move_to_next_state(scl_changed, sda_changed)
