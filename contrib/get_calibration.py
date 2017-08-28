#!/usr/bin/env python3

from argparse import ArgumentParser


if __name__ == '__main__':
    parser = ArgumentParser(description='Calculate gain and offset from the first two photopeaks.')
    parser.add_argument('thr_1pe', type=float)
    parser.add_argument('thr_2pe', type=float)
    args = parser.parse_args()

    gain = args.thr_2pe - args.thr_1pe
    offset = args.thr_1pe - gain

    print(gain)
    print(offset)
