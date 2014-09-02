universal-machine-emulator
==========================

Emulator for the Universal Machine challenge - CS554 Warmup Project


Building
--------
gcc -m32 -o emulator main.c

Not compiling for 32-bit architecture (-m32) may or may not cause bugs during execution. It will definitely cause gcc to throw warnings when compiling.

Running
-------
./emulator program
