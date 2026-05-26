#!/usr/bin/env python3
"""
GRBL Simulator for CYD CNC Controller Testing

Connects to the CYD's USB serial port and simulates a GRBL 1.1 controller.
The CYD must be compiled with DEBUG_SERIAL_GRBL=1 so that GRBL communication
goes over USB Serial instead of UART2.

The simulator:
- Sends startup banner on connect
- Responds to '?' with a status report (simulated position)
- Acknowledges GCode commands with 'ok' after a configurable delay (default 200ms)
- Ignores debug/log lines from ESP32 (lines starting with '[')
- Handles realtime commands (feed hold, cycle resume, soft reset, jog cancel)
- Simulates position changes from G0/G1 and jog ($J=) commands

Usage:
    python3 tools/grbl_simulator.py [--port /dev/ttyUSB1] [--baud 115200] [--delay 0.2]
"""

import argparse
import re
import serial
import sys
import threading
import time


class GrblSimulator:
    def __init__(self, port, baud, delay):
        self.port = port
        self.baud = baud
        self.ack_delay = delay

        # Simulated machine state
        self.state = "Idle"
        self.wpos = [0.0, 0.0, 0.0]  # X, Y, Z
        self.feed_rate = 0
        self.spindle_speed = 0
        self.feed_override = 100
        self.rapid_override = 100
        self.spindle_override = 100
        self.buf_blocks = 15
        self.buf_rx = 128

        self.running = True
        self.lock = threading.Lock()

    def run(self):
        print(f"[SIM] Opening {self.port} at {self.baud} baud")
        print(f"[SIM] Ack delay: {self.ack_delay*1000:.0f}ms")
        print(f"[SIM] Press Ctrl+C to quit\n")

        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.05)
        except serial.SerialException as e:
            print(f"[SIM] ERROR: {e}")
            sys.exit(1)

        # Send GRBL startup banner
        time.sleep(0.5)
        self.send("\r\nGrbl 1.1h ['$' for help]\r\n")
        print("[SIM] Sent startup banner")

        rx_buf = ""

        try:
            while self.running:
                # Check for incoming data
                data = self.ser.read(256)
                if not data:
                    continue

                for byte in data:
                    c = chr(byte)

                    # Handle realtime characters (single byte, no newline)
                    if byte == 0x18:  # Ctrl-X = Soft Reset
                        print("[SIM] << SOFT RESET (0x18)")
                        self.handle_soft_reset()
                        rx_buf = ""
                        continue
                    elif c == '?':
                        self.handle_status_query()
                        continue
                    elif c == '!':
                        print("[SIM] << FEED HOLD")
                        with self.lock:
                            self.state = "Hold:0"
                        continue
                    elif c == '~':
                        print("[SIM] << CYCLE RESUME")
                        with self.lock:
                            self.state = "Idle"
                        continue
                    elif byte == 0x85:  # Jog Cancel
                        print("[SIM] << JOG CANCEL (0x85)")
                        with self.lock:
                            self.state = "Idle"
                        continue

                    # Buffer line characters
                    if c == '\n':
                        line = rx_buf.strip()
                        rx_buf = ""
                        if line:
                            self.handle_line(line)
                    elif c != '\r':
                        rx_buf += c

        except KeyboardInterrupt:
            print("\n[SIM] Shutting down")
        finally:
            self.ser.close()

    def send(self, msg):
        self.ser.write(msg.encode('ascii'))

    def handle_soft_reset(self):
        with self.lock:
            self.state = "Idle"
            self.feed_rate = 0
            self.spindle_speed = 0
        time.sleep(0.5)
        self.send("\r\nGrbl 1.1h ['$' for help]\r\n")
        print("[SIM] Sent reset banner")

    def handle_status_query(self):
        with self.lock:
            status = (
                f"<{self.state}"
                f"|WPos:{self.wpos[0]:.3f},{self.wpos[1]:.3f},{self.wpos[2]:.3f}"
                f"|Bf:{self.buf_blocks},{self.buf_rx}"
                f"|FS:{self.feed_rate},{self.spindle_speed}"
                f"|Ov:{self.feed_override},{self.rapid_override},{self.spindle_override}>"
            )
        self.send(status + "\r\n")

    def handle_line(self, line):
        # Ignore debug/log messages from the ESP32 (lines starting with '[')
        if line.startswith('['):
            print(f"[SIM] -- ignore debug: {line}")
            return

        # Ignore empty lines
        if not line:
            return

        print(f"[SIM] << {line}")

        # GRBL settings query
        if line == '$$':
            self.send_settings()
            return

        # Help
        if line == '$':
            self.send("[HLP:$$ $# $G $I $N $C $X $H ~ ! ? ctrl-x]\r\n")
            self.send("ok\r\n")
            return

        # Homing
        if line == '$H':
            print("[SIM]    Homing...")
            with self.lock:
                self.state = "Home"
            time.sleep(self.ack_delay)
            with self.lock:
                self.wpos = [0.0, 0.0, 0.0]
                self.state = "Idle"
            self.send("ok\r\n")
            print("[SIM] >> ok")
            return

        # Unlock
        if line == '$X':
            with self.lock:
                self.state = "Idle"
            time.sleep(self.ack_delay)
            self.send("[MSG:Caution: Unlocked]\r\n")
            self.send("ok\r\n")
            print("[SIM] >> ok")
            return

        # Jog command: $J=G91 X1.000 Y0.000 Z0.000 F1000
        if line.startswith('$J='):
            self.handle_jog(line[3:])
            return

        # GCode command — parse and acknowledge
        self.handle_gcode(line)

    def handle_jog(self, gcode):
        dx, dy, dz = self.parse_xyz(gcode)
        feed = self.parse_param(gcode, 'F')

        with self.lock:
            self.state = "Jog"
            if 'G91' in gcode:  # Relative
                self.wpos[0] += dx
                self.wpos[1] += dy
                self.wpos[2] += dz
            else:  # Absolute
                if dx != 0: self.wpos[0] = dx
                if dy != 0: self.wpos[1] = dy
                if dz != 0: self.wpos[2] = dz
            if feed:
                self.feed_rate = int(feed)

        time.sleep(self.ack_delay)

        with self.lock:
            self.state = "Idle"

        self.send("ok\r\n")
        print(f"[SIM] >> ok  (pos: X={self.wpos[0]:.3f} Y={self.wpos[1]:.3f} Z={self.wpos[2]:.3f})")

    def handle_gcode(self, line):
        # Parse motion commands to update simulated position
        dx, dy, dz = self.parse_xyz(line)
        feed = self.parse_param(line, 'F')
        spindle = self.parse_param(line, 'S')

        with self.lock:
            prev_state = self.state

            # Detect G90/G91
            is_relative = 'G91' in line

            # Update position for G0/G1 moves
            if any(c in line.upper() for c in ['X', 'Y', 'Z']) and \
               any(line.upper().startswith(g) for g in ['G0', 'G1', 'G90', 'G91']):
                if is_relative:
                    self.wpos[0] += dx
                    self.wpos[1] += dy
                    self.wpos[2] += dz
                else:
                    if dx != 0 or 'X' in line.upper(): self.wpos[0] = dx
                    if dy != 0 or 'Y' in line.upper(): self.wpos[1] = dy
                    if dz != 0 or 'Z' in line.upper(): self.wpos[2] = dz
                self.state = "Run"

            if feed:
                self.feed_rate = int(feed)
            if spindle:
                self.spindle_speed = int(spindle)

            # G10 L20 — set work coordinate zero
            if 'G10' in line and 'L20' in line:
                if 'X0' in line: self.wpos[0] = 0.0
                if 'Y0' in line: self.wpos[1] = 0.0
                if 'Z0' in line: self.wpos[2] = 0.0

            # M3/M4 spindle on, M5 spindle off
            if 'M5' in line:
                self.spindle_speed = 0
            # M2/M30 program end
            if 'M2' in line or 'M30' in line:
                self.state = "Idle"
                self.spindle_speed = 0

        # Simulate execution delay
        time.sleep(self.ack_delay)

        with self.lock:
            if self.state == "Run":
                self.state = "Idle"

        self.send("ok\r\n")
        pos_str = f"X={self.wpos[0]:.3f} Y={self.wpos[1]:.3f} Z={self.wpos[2]:.3f}"
        print(f"[SIM] >> ok  ({pos_str})")

    def parse_xyz(self, gcode):
        x = self.parse_param(gcode, 'X') or 0.0
        y = self.parse_param(gcode, 'Y') or 0.0
        z = self.parse_param(gcode, 'Z') or 0.0
        return x, y, z

    @staticmethod
    def parse_param(gcode, letter):
        match = re.search(rf'{letter}([+-]?\d*\.?\d+)', gcode, re.IGNORECASE)
        if match:
            return float(match.group(1))
        return None

    def send_settings(self):
        """Send common GRBL settings."""
        settings = [
            "$0=10", "$1=25", "$2=0", "$3=0", "$4=0", "$5=0",
            "$6=0", "$10=1", "$11=0.010", "$12=0.002",
            "$13=0", "$20=0", "$21=0", "$22=0",
            "$23=0", "$24=25.000", "$25=500.000",
            "$26=250", "$27=1.000",
            "$30=1000", "$31=0", "$32=0",
            "$100=250.000", "$101=250.000", "$102=250.000",
            "$110=500.000", "$111=500.000", "$112=500.000",
            "$120=10.000", "$121=10.000", "$122=10.000",
            "$130=200.000", "$131=200.000", "$132=200.000",
        ]
        for s in settings:
            self.send(s + "\r\n")
        self.send("ok\r\n")
        print("[SIM] >> settings + ok")


def main():
    parser = argparse.ArgumentParser(description="GRBL 1.1 Simulator for CYD CNC testing")
    parser.add_argument("--port", "-p", default="/dev/ttyUSB1",
                        help="Serial port (default: /dev/ttyUSB1 — CYD USB port)")
    parser.add_argument("--baud", "-b", type=int, default=115200,
                        help="Baud rate (default: 115200)")
    parser.add_argument("--delay", "-d", type=float, default=0.2,
                        help="Ack delay in seconds (default: 0.2)")
    args = parser.parse_args()

    sim = GrblSimulator(args.port, args.baud, args.delay)
    sim.run()


if __name__ == "__main__":
    main()

