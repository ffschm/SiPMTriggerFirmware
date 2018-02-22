#!/usr/bin/env python3

from argparse import ArgumentParser


if __name__ == '__main__':
    parser = ArgumentParser(description='Calculate offset and gain from the first two photopeaks.')
    parser.add_argument('-c', '--channel', type=int)
    parser.add_argument('thr_1pe', type=float)
    parser.add_argument('thr_2pe', type=float)
    args = parser.parse_args()

    gain = args.thr_2pe - args.thr_1pe
    offset = args.thr_1pe - gain

    print("Offset: {}".format(offset))
    print("Gain:   {}".format(gain))

    if (args.channel and args.channel in [1,2]):
        print("\nSend these commands to the SiPMTrigger Controller to set the gain and offset of this channel:")
        print("SET OFFSET {}, {}".format(args.channel, offset))
        print("SET GAIN {}, {}".format(args.channel, gain))
