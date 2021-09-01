# pico C program for Cortex-M microcontrollers

Based on https://github.com/noahp/minimal-c-cortex-m , this is a smaller version
that uses a single C file.

## memcpy testing

To run the test on the stm32f4-discovery board:

1. install openocd (and gcc-arm tools)
2. connect the usb mini cable to the board
3. in one terminal, run `make debug` to start openocd
4. in another terminal, run `make gdb` to build the application and load it over
   gdb
5. type `continue` to start the program in gdb
6. look for the semihosting output in the openocd terminal

To test the alternate `memcpy` implementation, clean and use `FAST_MEMCPY=1 make
gdb`.
