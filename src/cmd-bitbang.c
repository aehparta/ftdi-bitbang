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

const char opts[] = "hV:P:D:S:I:s:c:r";
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
	{ "read", no_argument, NULL, 'r' },
	{ 0, 0, 0, 0 },
};

/* usb vid (defaults to FT2232H) */
uint16_t usb_vid = 0x0403;
/* usb pid (defaults to FT2232H) */
uint16_t usb_pid = 0x6010;
/* usb description */
const char *usb_description = NULL;
/* usb serial */
const char *usb_serial = NULL;
/* interface (defaults to first one) */
int interface = INTERFACE_ANY;
int read_pins = 0;
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
	    "No help available.\n"
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
			read_pins = 1;
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

/**
 * Initialize resources needed by this process.
 *
 * @param argc Argument count.
 * @param argv Argument array.
 * @return 0 on success, -1 on errors.
 */
int p_init(int argc, char *argv[])
{
	int err = 0, i;

	for (i = 0; i < 16; i++) {
		pins[i] = -1;
	}

	/* parse command line options */
	if (p_options(argc, argv)) {
		fprintf(stderr, "invalid command line option(s)\n");
		return -1;
	}

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return -1;
	}
	err = ftdi_set_interface(ftdi, interface);
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}
	err = ftdi_usb_open_desc(ftdi, usb_vid, usb_pid, usb_description, usb_serial);
	if (err < 0) {
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		return -1;
	}
	ftdi_bitbang_load_state(device);

	return 0;
}


int main(int argc, char *argv[])
{
	int err = 0, i;

	if (p_init(argc, argv)) {
		p_exit(EXIT_FAILURE);
	}

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
	if (read_pins) {
		int pins = ftdi_bitbang_read(device);
		printf("%4x\n", pins);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

