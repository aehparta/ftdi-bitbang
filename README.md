# Contents
## Command line tools
* ftdi-bitbang
* ftdi-control
* ftdi-hd44780
## Libraries
* libftdi-bitbang
* libftdi-hd44780

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
  -R, --reset                do usb reset on the device at start

  -E, --ee-erase             erase eeprom, sometimes needed if eeprom has already been initialized
  -N, --ee-init              erase and initialize eeprom with defaults
  -o, --ee-decode            read eeprom and print decoded information
  -m, --ee-manufacturer=STRING
                             write manufacturer string
  -d, --ee-description=STRING
                             write description (product) string
  -s, --ee-serial=STRING     write serial string
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
