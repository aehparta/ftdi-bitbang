/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "m:s:c:i:r";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "mode", required_argument, NULL, 'm' },
	{ "set", required_argument, NULL, 's' },
	{ "clr", required_argument, NULL, 'c' },
	{ "inp", required_argument, NULL, 'i' },
	{ "read", optional_argument, NULL, 'r' },
	{ 0, 0, 0, 0 },
};

/* which pin state to read or -1 for all (hex output then) */
int read_pin = -2;
int pins[16];

/* ftdi device context */
struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_context *device = NULL;
int bitmode = 0;

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

void p_help()
{
	printf(
	    "  -m, --mode=STRING          set device bitmode, use 'bitbang' or 'mpsse', default is 'bitbang'\n"
	    "                             for bitbang mode the baud rate is fixed to 1 MHz for now\n"
	    "  -s, --set=PIN              given pin as output and one\n"
	    "  -c, --clr=PIN              given pin as output and zero\n"
	    "  -i, --inp=PIN              given pin as input\n"
	    "                             multiple -s, -c and -i options can be given\n"
	    "  -r, --read                 read pin states, output hexadecimal word\n"
	    "      --read=PIN             read single pin, output binary 0 or 1\n"
	    "\n"
	    "Simple command line bitbang interface to FTDI FTx232 chips.\n"
	    "\n");
}

int p_options(int c, char *optarg)
{
	int i;
	switch (c) {
	case 'm':
		if (strcmp("bitbang", optarg) == 0) {
			bitmode = BITMODE_BITBANG;
		} else if (strcmp("mpsse", optarg) == 0) {
			bitmode = BITMODE_MPSSE;
		} else {
			fprintf(stderr, "invalid bitmode\n");
			return -1;
		}
		return 1;
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
		return 1;
	case 'r':
		read_pin = -1;
		if (optarg) {
			read_pin = atoi(optarg);
		}
		if (read_pin < -1 || read_pin > 15) {
			fprintf(stderr, "invalid pin number for read parameter: %d\n", i);
			p_exit(1);
		}
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0, i;

	for (i = 0; i < 16; i++) {
		pins[i] = -1;
	}

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = common_ftdi_init();
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi, bitmode, 1);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}

	/* write changes */
	for (i = 0; i < 16; i++) {
		int err = 0;
		if (pins[i] == 0 || pins[i] == 1) {
			err += ftdi_bitbang_set_io(device, i, 1);
			err += ftdi_bitbang_set_pin(device, i, pins[i] ? 1 : 0);
		} else if (pins[i] == 2) {
			err += ftdi_bitbang_set_io(device, i, 0);
			err += ftdi_bitbang_set_pin(device, i, 0);
		}
		if (err != 0) {
			fprintf(stderr, "invalid pin #%d (you are propably trying to use upper pins in bitbang mode)\n", i);
		}
	}
	if (ftdi_bitbang_write(device) < 0) {
		fprintf(stderr, "failed writing pin states\n");
		p_exit(EXIT_FAILURE);
	}

	/* read pins */
	if (read_pin == -1) {
		int pins = ftdi_bitbang_read(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin states\n");
			p_exit(EXIT_FAILURE);
		}
		printf("%04x\n", pins);
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

