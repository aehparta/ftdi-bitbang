/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = "hV:P:D:S:I:s:c:i:r";
struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "vid", required_argument, NULL, 'V' },
	{ "pid", required_argument, NULL, 'P' },
	{ "description", required_argument, NULL, 'D' },
	{ "serial", required_argument, NULL, 'S' },
	{ "interface", required_argument, NULL, 'I' },
	{ "set", required_argument, NULL, 's' },
	{ "clr", required_argument, NULL, 'c' },
	{ "inp", required_argument, NULL, 'i' },
	{ "read", optional_argument, NULL, 'r' },
	{ 0, 0, 0, 0 },
};

/* usb vid */
uint16_t usb_vid = 0;
/* usb pid */
uint16_t usb_pid = 0;
/* usb description */
const char *usb_description = NULL;
/* usb serial */
const char *usb_serial = NULL;
/* interface (defaults to first one) */
int interface = INTERFACE_ANY;
/* which pin state to read or -1 for all (hex output then) */
int read_pin = -2;
int pins[16];

struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_dev *device = NULL;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
	if (device) {
		ftdi_bitbang_save_state(device);
		ftdi_bitbang_free(device);
	}
	if (ftdi) {
		ftdi_free(ftdi);
	}
	/* terminate program instantly */
	exit(return_code);
}

/**
 * Print commandline help.
 */
void p_help(void)
{
	printf(
	    "Usage: ftdi-bitbang [options]\n"
	    " -h, --help                   show this help\n"
	    " -V, --vid=ID                 usb vendor id\n"
	    " -P, --pid=ID                 usb product id\n"
	    "                              as default vid and pid are zero, so any FTDI device is used\n"
	    " -D, --description=STRING     usb description (product) to use for opening right device, default none\n"
	    " -S, --serial=STRING          usb serial to use for opening right device, default none\n"
	    " -I, --interface=INTERFACE    ftx232 interface(1-4) number, default first\n"
	    " -s, --set=PIN                given pin(0-15) as output and one\n"
	    " -c, --clr=PIN                given pin(0-15) as output and zero\n"
	    " -i, --inp=PIN                given pin(0-15) as input\n"
	    " -r, --read[=PIN]             read pin states, output as hex word, if --read=<0-15> given, will output 0 or 1 for that pin\n"
	    "\n");
}

/**
 * Print commandline help.
 */
int p_options(int argc, char *argv[])
{
	int err = 0;
	int longindex = 0, c;
	int i;

	while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
		switch (c) {
		case 'V':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				fprintf(stderr, "invalid usb vid value\n");
				p_exit(1);
			}
			usb_vid = (uint16_t)i;
			break;
		case 'P':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				fprintf(stderr, "invalid usb pid value\n");
				p_exit(1);
			}
			usb_pid = (uint16_t)i;
			break;
		case 'D':
			usb_description = strdup(optarg);
			break;
		case 'S':
			usb_serial = strdup(optarg);
			break;
		case 'I':
			interface = atoi(optarg);
			if (interface < 0 || interface > 4) {
				fprintf(stderr, "invalid interface\n");
				p_exit(1);
			}
			break;
		case 'c':
		case 's':
		case 'i':
			i = atoi(optarg);
			if (i < 0 || i > 15) {
				fprintf(stderr, "invalid pin number: %d\n", i);
				p_exit(1);
			} else {
				/* s = out&one, c = out&zero, i = input */
				pins[i] = c == 's' ? 1 : (c == 'i' ? 2 : 0);
			}
			break;
		case 'r':
			read_pin = -1;
			if (optarg) {
				read_pin = atoi(optarg);
			}
			if (read_pin < -1 || read_pin > 15) {
				fprintf(stderr, "invalid pin number for read parameter: %d\n", i);
				p_exit(1);
			}
			break;
		default:
		case '?':
		case 'h':
			p_help();
			p_exit(1);
		}
	}

out_err:
	return err;
}

int main(int argc, char *argv[])
{
	int err = 0, i;

	for (i = 0; i < 16; i++) {
		pins[i] = -1;
	}
	
	/* parse command line options */
	if (p_options(argc, argv)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = cmd_init(usb_vid, usb_pid, usb_description, usb_serial, interface);
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}
	ftdi_bitbang_load_state(device);

	/* write changes */
	for (i = 0; i < 16; i++) {
		if (pins[i] == 0 || pins[i] == 1) {
			ftdi_bitbang_set_io(device, i, 1);
			ftdi_bitbang_set_pin(device, i, pins[i] ? 1 : 0);
		} else if (pins[i] == 2) {
			ftdi_bitbang_set_io(device, i, 0);
			ftdi_bitbang_set_pin(device, i, 0);
		}
	}
	ftdi_bitbang_write(device);

	/* read pins */
	if (read_pin == -1) {
		int pins = ftdi_bitbang_read(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin states\n");
			p_exit(EXIT_FAILURE);
		}
		printf("%4x\n", pins);
	} else if (read_pin > 0 && read_pin < 8) {
		int pins = ftdi_bitbang_read_low(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin state\n");
			p_exit(EXIT_FAILURE);
		}
		printf("%d\n", pins & (1 << read_pin) ? 1 : 0);
	} else if (read_pin > 7 && read_pin < 16) {
		int pins = ftdi_bitbang_read_high(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin state\n");
			p_exit(EXIT_FAILURE);
		}
		read_pin -= 8;
		printf("%d\n", pins & (1 << read_pin) ? 1 : 0);
	}


	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

