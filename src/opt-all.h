/*
 * All options used by commands in one simple block.
 * Not the coolest or cleanest way to do this but works.
 *
 * Only include this file once in one place!
 */

#include <stdlib.h>
#include "ftdic.h"
#include "opt.h"


int opt_L_callback(int short_name, char *value)
{
	ftdic_list();
	opt_quit();
	exit(EXIT_SUCCESS);
}

const char *opt_M_accept[] = { "bitbang", "mpsse", NULL };

struct opt_option opt_all[] = {
    { 'h', "help", no_argument, 0, NULL, NULL, "display this help and exit", { 0 } },
    {
        'V', "vid", required_argument, 0, "0", NULL, "usb vendor id",
        { OPT_FILTER_HEX, 0x0000, 0xffff }
    },
    {
        'P', "pid", required_argument, 0, "0", NULL,
        "usb product id\n"
        "when vid and pid are zero, any first ftdi device found is used",
        { OPT_FILTER_HEX, 0x0000, 0xffff }
    },
    { 'D', "description", required_argument, 0, NULL, NULL, "usb description (product) to use for opening right device", { 0 } },
    { 'S', "serial", required_argument, 0, NULL, NULL, "usb serial to use for opening right device", { 0 } },
    { 'U', "usbid", required_argument, 0, NULL, NULL, "usbid to use for opening right device (sysfs format, e.g. 1-2.3)", { 0 } },
    {
        'I', "interface", required_argument, 0, "1", NULL, "ftx232 interface number, defaults to first",
        { OPT_FILTER_INT, 1, 4 }
    },
    { 'R', "reset", no_argument, 0, NULL, NULL, "do usb reset on the device at start", { 0 } },
    { 'L', "list", no_argument, 0, NULL, opt_L_callback, "list devices that can be found with given parameters", { 0 } },
    {
        'M', "mode", required_argument, 0, "bitbang", NULL, "set device bitmode, use 'bitbang' or 'mpsse'",
        { OPT_FILTER_STR, NAN, NAN, opt_M_accept }
    },
    { 'B', "baudrate", required_argument, 0, NULL, NULL, "set device baudrate, will default to what ever the device is using", { OPT_FILTER_INT, 0, 20e6 } },

    /* ftdi-control only */
    { 'W', "serial-len", required_argument, 0, NULL, NULL, "pad serial with randomized ascii letters and numbers to this length (upper case)", { 0 } },
    { 'X', "serial-hex", required_argument, 0, NULL, NULL, "pad serial with randomized hex to this length (upper case)", { 0 } },
    { 'C', "serial-dec", required_argument, 0, NULL, NULL, "pad serial with randomized numbers to this length", { 0 } },
    { 'F', "fix-broken", no_argument, 0, NULL, NULL, "try to fix broken device with vid:pid set to 0:0 or eeprom checksum gone wild", { 0 } },

    /* ftdi-hd44780 only */
    { '4', "d4", required_argument, 0, NULL, NULL, "data pin 4, default pin is 0", { OPT_FILTER_INT, 0, 15 } },
    { '5', "d5", required_argument, 0, NULL, NULL, "data pin 5, default pin is 1", { OPT_FILTER_INT, 0, 15 } },
    { '6', "d6", required_argument, 0, NULL, NULL, "data pin 6, default pin is 2", { OPT_FILTER_INT, 0, 15 } },
    { '7', "d7", required_argument, 0, NULL, NULL, "data pin 7, default pin is 3", { OPT_FILTER_INT, 0, 15 } },
    { 'e', "en", required_argument, 0, NULL, NULL, "enable pin, default pin is 4", { OPT_FILTER_INT, 0, 15 } },
    { 'r', "rw", required_argument, 0, NULL, NULL, "read/write pin, default pin is 5", { OPT_FILTER_INT, 0, 15 } },
    { 's', "rs", required_argument, 0, NULL, NULL, "register select pin, default pin is 6", { OPT_FILTER_INT, 0, 15 } },

    { 0, 0, 0, 0, 0, 0, 0, { 0 } }
};
