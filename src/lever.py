import serial
import os
import time
from typing import Tuple, Optional

#   https://github.com/pyserial/pyserial/blob/master/serial/tools/list_ports.py
if os.name == 'nt':  # sys.platform == 'win32':
    from serial.tools.list_ports_windows import comports
elif os.name == 'posix':
    from serial.tools.list_ports_posix import comports
else:
    raise ImportError("Sorry: no implementation for your platform ('{}') available".format(os.name))

POST_WRITE_TIMEOUT_S = 0.025   #   Sleep for n seconds after writing

def enumerate_ports():
    hits = 0
    iterator = sorted(comports(include_links=False))
    res = []
    for n, (port, desc, hwid) in enumerate(iterator, 1):
        res.append({
            'name': port,
            'desc': '{}'.format(desc),
            'hwid': '{}'.format(hwid)
        })
        hits += 1
    return res

def has_port_with_name(ports, name: str):
    for p in ports:
        if p['name'] == name:
            return True
    return False

class SerialContext(object):
    def __init__(self, port: str, baud_rate: int = 9600, timeout: int = 1):
        self.port = port
        self.baud_rate = baud_rate
        self.is_open = False
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = baud_rate
        self.ser.bytesize = 8
        self.ser.timeout = timeout

    def open_sync(self, timeout_s: float = 4.0) -> bool:
        success = False
        if not self.is_open:
            self.ser.open()
            self.is_open = True
            time.sleep(timeout_s)
            success = True
        return success

    def close(self):
        if self.is_open:
            self.ser.close()
            self.is_open = False

    def write(self, data: bytes):
        if self.is_open:
            self.ser.write(data)
            return True
        return False

    def readline(self) -> Tuple[bool, bytes]:
        if not self.is_open:
            return False, bytes()
        data = self.ser.readline()
        complete_line = b'\n' in data
        return complete_line, data

def make_default_opened_context(port: str) -> Tuple[Optional[str], Optional[SerialContext]]:
    ports = enumerate_ports()
    if not has_port_with_name(ports, port):
        return 'No such port: {}'.format(port), None

    ctx = SerialContext(port)
    success = ctx.open_sync()
    if not success:
        return 'Failed to open connection on port: {}'.format(port), None
    
    return None, ctx

def write_timeout(ctx: SerialContext, data: bytes, sync: bool = True):
    ctx.write(data)
    if sync:
        time.sleep(POST_WRITE_TIMEOUT_S)

def set_enabled(ctx: SerialContext, sync: bool = True):
    write_timeout(ctx, b'x', sync)

def set_disabled(ctx: SerialContext, sync: bool = True):
    write_timeout(ctx, b'o', sync)

def set_force_grams(ctx: SerialContext, force: int, sync: bool = True) -> Tuple[bool, bytes]:
    write_timeout(ctx, bytes(f'g{force}', 'utf-8'), sync)
    return ctx.readline()

def read_state(ctx: SerialContext, sync: bool = True) -> Tuple[bool, bytes]:
    write_timeout(ctx, b's', sync)
    return ctx.readline()

def parse_read_state(data: bytes) -> Tuple[bool, float, float, float]:
    split = str(data, 'utf-8').split('\t')

    strain_read = 0.0
    calc_pwm = 0.0
    actual_pwm = 0.0
    if not len(split) == 3:
        return False, strain_read, calc_pwm, actual_pwm

    #   Strain gauge reading
    sg_split = split[0].split('strain gauge reading: ')
    if not len(sg_split) == 2:
        return False, strain_read, calc_pwm, actual_pwm
    else:
        strain_read = float(sg_split[1])

    #   Internally computed PWM
    c_pwm_split = split[1].split('calculated PWM: ')
    if not len(c_pwm_split) == 2:
        return False, strain_read, calc_pwm, actual_pwm
    else:
        calc_pwm = float(c_pwm_split[1])

    #   Actual (commanded) PWM
    a_pwm_split = split[2].split('acutal PWM: ')  #   note, typo
    if not len(a_pwm_split) == 2:
        return False, strain_read, calc_pwm, actual_pwm
    else:
        actual_pwm = float(a_pwm_split[1])

    return True, strain_read, calc_pwm, actual_pwm