#!/usr/bin/env python3

from argparse import ArgumentParser
import serial
import sys

def wait_until_string(ser, string):
    while True:
        line = ser.readline()
        if len(line) > 0 and line == string.encode('ascii'):
            break


def print_until_string(ser, string, file=sys.stderr):
    while True:
        line = ser.readline()
        if len(line) > 0 and line == string.encode('ascii'):
            break
        print(line.decode('ascii').strip(), file=file)

def thr_scan(ser, channel):
    ser.write('SCAN THR {}\n'.format(channel).encode('ascii'))

    wait_until_string(ser, '# Threshold scan for a single channel started. Please wait...\r\n')

    try:
        print_until_string(ser, '# Threshold scan finished.\r\n')
    except KeyboardInterrupt:
        ser.write(b'\n')
        print_until_string(ser, '# Threshold scan aborted.\r\n')
        pass

    print("# THR_CH1/p.e. THR_CH2/p.e. R/Hz deltaR/Hz", file=sys.stdout)
    print_until_string(ser, '# END Spectrum.\r\n', sys.stdout)


if __name__ == '__main__':
    parser = ArgumentParser(description='Run a treshold scan on the discriminator of the SiPM Trigger Controller.')
    parser.add_argument('--port', default='/dev/ttyACM0')
    parser.add_argument('--channel', type=int, default=1)
    args = parser.parse_args()
    
    ser = serial.Serial(args.port,
                        9600,
                        bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE,
                        stopbits=serial.STOPBITS_ONE,
                        xonxoff=True,
                        rtscts=True)

    # Discard incomplete input lines
    ser.flushInput()
    ser.flushOutput()
    ser.close()
    ser.open()

    thr_scan(ser, args.channel)

    ser.close()
