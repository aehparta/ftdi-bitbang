# Description
## Command line tools
* ftdi-bitbang
* ftdi-control
* ftdi-hd44780
* ftdi-simple-capture
## Libraries
* libftdi-bitbang
* libftdi-hd44780
## Compile
```sh
~/ftdi-bitbang$ ./autogen.sh
~/ftdi-bitbang$ make
~/ftdi-bitbang$ make install
```
Or in debian based platforms you can generate debian package:
```sh
~/ftdi-bitbang$ ./autogen.sh deb
~/ftdi-bitbang$ sudo dpkg -i debian/ftdi-bitbang-VERSION-BUILD-ARCH.deb
```

# ftdi-bitbang
Simple command line bitbang interface to FTDI FTx232 chips.
```
Usage:
 ftdi-bitbang [options]

Definitions for options:
 ID = hexadecimal word
 PIN = decimal between 0 and 15
 INTERFACE = integer between 1 and 4 depending on device type

Options:
  -h, --help                 display this help and exit
  -V, --vid=ID               usb vendor id
  -P, --pid=ID               usb product id
                             as default vid and pid are zero, so any first compatible ftdi device is used
  -D, --description=STRING   usb description (product) to use for opening right device, default none
  -S, --serial=STRING        usb serial to use for opening right device, default none
  -I, --interface=INTERFACE  ftx232 interface number, defaults to first
  -R, --reset                do usb reset on the device at start

  -m, --mode=STRING          set device bitmode, use 'bitbang' or 'mpsse', default is 'bitbang'
                             for bitbang mode the baud rate is fixed to 1 MHz for now
  -s, --set=PIN              given pin as output and one
  -c, --clr=PIN              given pin as output and zero
  -i, --inp=PIN              given pin as input
                             multiple -s, -c and -i options can be given
  -r, --read                 read pin states, output hexadecimal word
      --read=PIN             read single pin, output binary 0 or 1
```


# ftdi-control
Basic control and eeprom routines for FTDI FTx232 chips.
```
Usage:
 ftdi-control [options]

Definitions for options:
 ID = hexadecimal word
 PIN = decimal between 0 and 15
 INTERFACE = integer between 1 and 4 depending on device type

Options:
  -h, --help                 display this help and exit
  -V, --vid=ID               usb vendor id
  -P, --pid=ID               usb product id
                             as default vid and pid are zero, so any first compatible ftdi device is used
  -D, --description=STRING   usb description (product) to use for opening right device, default none
  -S, --serial=STRING        usb serial to use for opening right device, default none
  -I, --interface=INTERFACE  ftx232 interface number, defaults to first
  -U, --usbid=ID             usbid to use for opening right device (sysfs format, e.g. 1-2.3), default none
  -R, --reset                do usb reset on the device at start
  -L, --list                 list devices that can be found with given parameters

  -E, --ee-erase             erase eeprom, sometimes needed if eeprom has already been initialized
  -N, --ee-init              erase and initialize eeprom with defaults
  -o, --ee-decode            read eeprom and print decoded information
  -m, --ee-manufacturer=STRING
                             write manufacturer string
  -d, --ee-description=STRING
                             write description (product) string
  -s, --ee-serial=STRING     write serial string
  -l, --ee-serial-len=LENGTH pad serial with randomized ascii letters and numbers to this length (upper case)
  -x, --ee-serial-hex=LENGTH pad serial with randomized hex to this length (upper case)
  -n, --ee-serial-dec=LENGTH pad serial with randomized numbers to this length
  -p, --ee-bus-power=INT     bus power drawn by the device (100-500 mA)
```


# ftdi-hd44780
Use HD44780 based LCD displays in 4-bit mode through FTDI FTx232 chips with this command.
```
Usage:
 ftdi-hd44780 [options]

Definitions for options:
 ID = hexadecimal word
 PIN = decimal between 0 and 15
 INTERFACE = integer between 1 and 4 depending on device type

Options:
  -h, --help                 display this help and exit
  -V, --vid=ID               usb vendor id
  -P, --pid=ID               usb product id
                             as default vid and pid are zero, so any first compatible ftdi device is used
  -D, --description=STRING   usb description (product) to use for opening right device, default none
  -S, --serial=STRING        usb serial to use for opening right device, default none
  -I, --interface=INTERFACE  ftx232 interface number, defaults to first
  -R, --reset                do usb reset on the device at start

  -m, --mode=STRING          set device bitmode, use 'bitbang' or 'mpsse', default is 'bitbang'
                             for bitbang mode the baud rate is fixed to 1 MHz for now
  -i, --init                 initialize hd44780 lcd, usually needed only once at first
  -4, --d4=PIN               data pin 4, default pin is 0
  -5, --d5=PIN               data pin 5, default pin is 1
  -6, --d6=PIN               data pin 6, default pin is 2
  -7, --d7=PIN               data pin 7, default pin is 3
  -e, --en=PIN               enable pin, default pin is 4
  -r, --rw=PIN               read/write pin, default pin is 5
  -s, --rs=PIN               register select pin, default pin is 6
  -b, --command=BYTE         send raw hd44780 command, decimal or hexadecimal (0x) byte
                             multiple commands can be given, they are run before any later commands described here
  -C, --clear                clear display
  -M, --home                 move cursor home
  -c, --cursor=VALUE         cursor on/off and blink, accepts value between 0-3
  -t, --text=STRING          write given text to display
  -l, --line=VALUE           move cursor to given line, value between 0-3
```

# ftdi-simple-capture
Do simple digital data capturing using FTDI FTx232 chips.
Can be used to analyze digital circuit with or without trigger.
Example use case would be to capture IR signal at pin #0,
trigger on rising edge and capture 500 mS,
then feeding it to external script which would filter it as ones and zeroes and so on.
```
Usage:
 ftdi-simple-capture [options]

Definitions for options:
 ID = hexadecimal word
 PIN = decimal between 0 and 15
 INTERFACE = integer between 1 and 4 depending on device type

Options:
  -h, --help                 display this help and exit
  -V, --vid=ID               usb vendor id
  -P, --pid=ID               usb product id
                             as default vid and pid are zero, so any first compatible ftdi device is used
  -D, --description=STRING   usb description (product) to use for opening right device, default none
  -S, --serial=STRING        usb serial to use for opening right device, default none
  -I, --interface=INTERFACE  ftx232 interface number, defaults to first
  -U, --usbid=ID             usbid to use for opening right device (sysfs format, e.g. 1-2.3), default none
  -R, --reset                do usb reset on the device at start
  -L, --list                 list devices that can be found with given parameters

  -p, --pins=PINS[0-7]       pins to capture, default is '0,1,2,3,4,5,6,7'
  -t, --trigger=PIN[0-7]:EDGE
                             trigger from pin on rising or falling edge (EDGE = r or f),
                             if trigger is not set, sampling will start immediately
  -s, --speed=INT            sampling speed, default 1000000 (1 MS/s)
  -l, --time=FLOAT           sample for this many seconds, default 1 second

Simple capture command for FTDI FTx232 chips.
Uses bitbang mode so only ADBUS (pins 0-7) can be sampled.

```

