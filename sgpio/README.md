Simple GPIO Linux wrappers
==========================

# Simple GPIO wrappers to input/output over /sys/class/gpio Linux file system

## Typical usage:

1. Call sgpio_unexport(gpio_number), may ignore errors first time.

2. Call sgpio_export(gpio_number), check return codes.

3. Run sgpio_init() as constructor of `sgpio_t` structure.

4. Run sgpio_mode() to set input/output and edge mode.

5. Call sgpio_set() for output or sgpio_get() for input.

6. For input lines with edge mode (rising, falling or both)
   may use sgpio_poll() or sgpio_epoll() functions.

