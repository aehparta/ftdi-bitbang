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
#include "ftdi-bitbang.h"

const char opts[] = "hi:1:0:";
struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "interface", required_argument, NULL, 'i' },
	{ "one", required_argument, NULL, '1' },
	{ "zero", required_argument, NULL, '0' },
	{ 0, 0, 0, 0 },
};

uint16_t vid = 0x0403;
uint16_t pid = 0x6010;
int interface = INTERFACE_ANY;

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
		ftdi_bitbang_free(device);
	}
	if (ftdi) {
		ftdi_usb_close(ftdi);
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
	int pin;

	while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
		switch (c) {
		case 'i':
			interface = atoi(optarg);
			interface = interface < 0 ? 0 : (interface > 4 ? 4 : interface);
			break;
		case '0':
			pin = atoi(optarg);
			if (pin < 0 || pin > 15) {
				fprintf(stderr, "invalid pin number: %d\n", pin);
				p_help();
				p_exit(1);
			} else {
				/* set as output and zero */
				ftdi_bitbang_set_io(device, pin, 1);
				ftdi_bitbang_set_pin(device, pin, 0);
			}
			break;
		case '1':
			pin = atoi(optarg);
			if (pin < 0 || pin > 15) {
				fprintf(stderr, "invalid pin number: %d\n", pin);
				p_help();
				p_exit(1);
			} else {
				/* set as output and one */
				ftdi_bitbang_set_io(device, pin, 1);
				ftdi_bitbang_set_pin(device, pin, 1);
			}
			break;
		default:
		case '?':
		case 'h':
			printf("%d\n", c);
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
	int err = 0;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return -1;
	}
	err = ftdi_usb_open(ftdi, vid, pid);
	if (err < 0) {
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}

	device = ftdi_bitbang_init(ftdi);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		return -1;
	}

	/* parse command line options */
	if (p_options(argc, argv)) {
		fprintf(stderr, "invalid command line options\n");
		return -1;
	}

	err = ftdi_set_interface(ftdi, interface);
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}

	/* write changes */
	ftdi_bitbang_write(device);

	return 0;
}


int main(int argc, char *argv[])
{
	int err = 0;

	if (p_init(argc, argv)) {
		fprintf(stderr, "failed to initialize\n");
		p_exit(EXIT_FAILURE);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

