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

def pe_thr_scan(ser):
    ser.write('SCAN PE THR\n'.encode('ascii'))

    wait_until_string(ser, '# Threshold scan started. Please wait...\r\n')
    try:
        print_until_string(ser, '# Threshold scan finished.\r\n')
    except KeyboardInterrupt:
        ser.write(b'\n')
        print_until_string(ser, '# Threshold scan aborted.\r\n')
        pass

    print("# THR_CH1/p.e. THR_CH2/p.e. R/Hz deltaR/Hz", file=sys.stdout)
    print_until_string(ser, '# END Spectrum.\r\n', sys.stdout)

def set_gain(ser, channel, gain):
    ser.write('SET GAIN {},{}\n'.format(channel, gain).encode('ascii'))

def set_offset(ser, channel, gain):
    ser.write('SET OFFSET {},{}\n'.format(channel, gain).encode('ascii'))

if __name__ == '__main__':
    parser = ArgumentParser(description='Run a treshold scan on the discriminator of the SiPM Trigger Controller.')
    parser.add_argument('--port', default='/dev/ttyACM0')
    parser.add_argument('--gain1', type=float)
    parser.add_argument('--offset1', type=float)
    parser.add_argument('--gain2', type=float)
    parser.add_argument('--offset2', type=float)
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

    if args.gain1:
        set_gain(ser, 1, args.gain1)

    if args.gain2:
        set_gain(ser, 2, args.gain2)

    if args.offset1:
        set_offset(ser, 1, args.offset1)

    if args.offset2:
        set_offset(ser, 2, args.offset2)

    pe_thr_scan(ser)

    ser.close()
