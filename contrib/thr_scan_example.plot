#!/usr/bin/gnuplot

set term postscript enhanced color eps
set output 'thr_scan_example.eps'

set logscale y
set grid

set xlabel 'Threshold / LSB'
set ylabel 'Rate / Hz'

plot "example_data/ch1.dat" w errorlines lt 1 pt 5, \
"example_data/ch2.dat" w errorlines lt 1 pt 4, \
"example_data/source_ch1.dat" w errorlines lt 2 pt 5, \
"example_data/source_ch2.dat" w errorlines lt 2 pt 4
