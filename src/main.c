/*
 * ftdi-hd44780
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "ftdi-hd44780.h"

const char opts[] = "hi:n4:5:6:7:e:r:s:cmt:l:";
struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "interface", required_argument, NULL, 'i' },
	{ "no-reset", no_argument, NULL, 'n' },
	{ "d4", required_argument, NULL, '4' },
	{ "d5", required_argument, NULL, '5' },
	{ "d6", required_argument, NULL, '6' },
	{ "d7", required_argument, NULL, '7' },
	{ "en", required_argument, NULL, 'e' },
	{ "rw", required_argument, NULL, 'r' },
	{ "rs", required_argument, NULL, 's' },
	{ "clear", no_argument, NULL, 'c' },
	{ "home", no_argument, NULL, 'm' },
	{ "text", required_argument, NULL, 't' },
	{ "line", required_argument, NULL, 'l' },
	{ 0, 0, 0, 0 },
};

uint16_t vid = 0x0403;
uint16_t pid = 0x6010;
int interface = INTERFACE_ANY;
int reset = 1;
int d4 = 0;
int d5 = 1;
int d6 = 2;
int d7 = 3;
int en = 4;
int rw = 5;
int rs = 6;

struct ftdi_context *ftdi = NULL;
struct ftdi_hd44780_dev *device = NULL;

int clear = 0;
int home = 0;
char *text = NULL;
int line = -1;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
	if (device) {
		ftdi_hd44780_free(device);
	}
	if (ftdi) {
		ftdi_usb_close(ftdi);
		ftdi_free(ftdi);
	}
	if (text) {
		free(text);
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

	while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
		switch (c) {
		case 'i':
			interface = atoi(optarg);
			interface = interface < 0 ? 0 : (interface > 4 ? 4 : interface);
			break;
		case 'n':
			reset = 0;
			break;
		case '4':
			d4 = atoi(optarg);
			break;
		case '5':
			d5 = atoi(optarg);
			break;
		case '6':
			d6 = atoi(optarg);
			break;
		case '7':
			d7 = atoi(optarg);
			break;
		case 'e':
			en = atoi(optarg);
			break;
		case 'r':
			rw = atoi(optarg);
			break;
		case 's':
			rs = atoi(optarg);
			break;
		case 'c':
			clear = 1;
			break;
		case 'm':
			home = 1;
			break;
		case 't':
			text = strdup(optarg);
			break;
		case 'l':
			line = atoi(optarg);
			line = line < 0 ? 0 : (line > 3 ? 3 : line);
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

	/* parse command line options */
	if (p_options(argc, argv)) {
		fprintf(stderr, "invalid command line options\n");
		return -1;
	}

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
	err = ftdi_set_interface(ftdi, interface);
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}

	device = ftdi_hd44780_init(ftdi, reset, d4, d5, d6, d7, en, rw, rs);
	if (!device) {
		fprintf(stderr, "ftdi_hd44780_init() failed\n");
		return -1;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int err = 0;

	if (p_init(argc, argv)) {
		fprintf(stderr, "failed to initialize\n");
		p_exit(EXIT_FAILURE);
	}

	if (clear) {
		ftdi_hd44780_cmd(device, 0x01);
	}
	if (home) {
		ftdi_hd44780_cmd(device, 0x02);
	}
	if (line >= 0 && line <= 3) {
		ftdi_hd44780_goto_xy(device, 0, line);
	}
	if (text) {
		ftdi_hd44780_write_str(device, text);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

