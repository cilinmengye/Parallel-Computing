#!/bin/bash

make clean
make
./mandelbrot -t 7
files=$(find . -name "*.txt")
for file in $files; do
	cat $file >> thread.txt
done
sort -k1,1n thread.txt -o thread.txt
