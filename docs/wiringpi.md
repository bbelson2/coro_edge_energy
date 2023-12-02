# Wiring Pi 
This library is deprecated.
However, it offers a minimal implementation for the GPIO requirements of this project.

Note that for builds on Windows, the macro `WIRINGPI_MOCK` is defined in the CMakeLists.txt file.

Installation on the Raspberry Pi platform follows the steps in [raspberrypi.md](../docs/raspberrypi.md).

The library is invoked via the class `gpio` in `gpio.h`.

For Windows, the class is a mock implementation, which writes a log to `gpio.csv` in the `data` directory.

