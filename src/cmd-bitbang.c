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

const char opts[] = COMMON_SHORT_OPTS "m:b:s:c:i:r:";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "mode", required_argument, NULL, 'm' },
	{ "baudrate", required_argument, NULL, 'b' },
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
int baudrate = 0;

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
	    "  -b, --baudrate=INT         set baudrate to given, default is 1MHz\n"
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
	case 'b':
		baudrate = atoi(optarg);
		if (baudrate < 1 || baudrate > 20e6) {
			fprintf(stderr, "invalid baudrate, maximum is 20MHz\n");
			p_exit(1);
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

int read_pins(int pin)
{
	if (pin == -1) {
		int pins = ftdi_bitbang_read(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin states\n");
			p_exit(EXIT_FAILURE);
		}
		printf("%04x\n", pins);
	} else if (pin >= 0 && pin < 8) {
		int pins = ftdi_bitbang_read_low(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin state\n");
			p_exit(EXIT_FAILURE);
		}
		printf("%d\n", pins & (1 << pin) ? 1 : 0);
	} else if (pin > 7 && pin < 16) {
		int pins = ftdi_bitbang_read_high(device);
		if (pins < 0) {
			fprintf(stderr, "failed reading pin state\n");
			p_exit(EXIT_FAILURE);
		}
		pin -= 8;
		printf("%d\n", pins & (1 << pin) ? 1 : 0);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0, changed = 0;

	for (int i = 0; i < 16; i++) {
		pins[i] = -1;
	}

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts, 0, 1)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = common_ftdi_init();
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi, bitmode, 1, baudrate);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}

	/* write changes */
	for (int i = 0; i < 16; i++) {
		int err = 0;
		if (pins[i] == 0 || pins[i] == 1) {
			err += ftdi_bitbang_set_io(device, i, 1);
			err += ftdi_bitbang_set_pin(device, i, pins[i] ? 1 : 0);
			changed++;
		} else if (pins[i] == 2) {
			err += ftdi_bitbang_set_io(device, i, 0);
			err += ftdi_bitbang_set_pin(device, i, 0);
			changed++;
		}
		if (err != 0) {
			fprintf(stderr, "invalid pin #%d (you are propably trying to use upper pins in bitbang mode)\n", i);
		}
	}
	if (ftdi_bitbang_write(device) < 0) {
		fprintf(stderr, "failed writing pin states\n");
		p_exit(EXIT_FAILURE);
	}

	/* read pin(s) if set so in options */
	if (read_pin > -2) {
		read_pins(read_pin);
		changed++;
	}

	/* apply arguments */
	// for (int i = optind; i < argc; i++) {
	// 	printf("use: %s\n", argv[i]);
	// }

	/* apply stdin */
	for (;;) {
		char **parts = common_stdin_parseline();
		if (!parts) {
			break;
		}
		printf("line:");
		for (int i = 0; parts[i]; i++) {
			printf(" '%s'", parts[i]);
		}
		printf("\n");
		changed++;
	}

	/* print error if nothing was done */
	if (!changed) {
		fprintf(stderr, "nothing done, no actions given\n");
		p_exit(EXIT_FAILURE);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

