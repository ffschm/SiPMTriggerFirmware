#!/usr/bin/gnuplot

set term postscript enhanced color eps
set output 'pe_thr_scan_example.eps'

set logscale y
set grid

set xlabel 'Threshold sum/p.e.'
set ylabel 'Rate/Hz'

plot 'example_data/pe_thr_scan_dark.dat' u (2*$1):2:3 w yerrorbars
