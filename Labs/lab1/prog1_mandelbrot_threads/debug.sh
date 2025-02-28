#!/bin/bash

make clean
make
./mandelbrot -t 2
cat thread0.txt >> thread.txt
cat thread1.txt >> thread.txt
diff serial.txt thread.txt > diff.txt

