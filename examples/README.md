# MicroRL library demo

## AVR demo

For AVR demo build type

```
$make -f Makefile.avr
```

Connect USART to PC (usb-convertor work!) COM-port and open terminal like `minicom` with it COM-port

```
$minicom -c on -D /dev/ttyUSB0 -b 115200
# -c on: turn on color in terminal
# -D /dev/ttyUSB0: your COM-port (virtual if usb-converter used)
# -b 11520: USART baud rate
```

| oscillator  |  baud-rate |
| -----------:|-----------:|
|    16MHz    |   115200   |
|     8MHz    |   57600    |
|     1MHz    |   14400    |

## Unix demo

For Unix demo build, just type

```
$make
```

