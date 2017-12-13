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
#include <errno.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "ftdi-hd44780.h"
#include "cmd-common.h"

const char opts[] = "hV:P:D:S:I:R4:5:6:7:e:r:s:b:CMc:t:l:";
struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "vid", required_argument, NULL, 'V' },
	{ "pid", required_argument, NULL, 'P' },
	{ "description", required_argument, NULL, 'D' },
	{ "serial", required_argument, NULL, 'S' },
	{ "interface", required_argument, NULL, 'I' },
	{ "reset", no_argument, NULL, 'R' },
	{ "d4", required_argument, NULL, '4' },
	{ "d5", required_argument, NULL, '5' },
	{ "d6", required_argument, NULL, '6' },
	{ "d7", required_argument, NULL, '7' },
	{ "en", required_argument, NULL, 'e' },
	{ "rw", required_argument, NULL, 'r' },
	{ "rs", required_argument, NULL, 's' },
	{ "command", required_argument, NULL, 'b' },
	{ "clear", no_argument, NULL, 'C' },
	{ "home", no_argument, NULL, 'M' },
	{ "cursor", required_argument, NULL, 'c' },
	{ "text", required_argument, NULL, 't' },
	{ "line", required_argument, NULL, 'l' },
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

int reset = 0;
int d4 = 0;
int d5 = 1;
int d6 = 2;
int d7 = 3;
int en = 4;
int rw = 5;
int rs = 6;

struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_dev *device = NULL;
struct ftdi_hd44780_dev *hd44780 = NULL;

uint8_t *commands = NULL;
int commands_count = 0;
int clear = 0;
int home = 0;
int cursor = -1;
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
	if (commands) {
		free(commands);
	}
	if (hd44780) {
		ftdi_hd44780_free(hd44780);
	}
	if (device) {
		ftdi_bitbang_save_state(device);
		ftdi_bitbang_free(device);
	}
	if (ftdi) {
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
	int err = 0, i;
	int longindex = 0, c;

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
		case 'R':
			reset = 1;
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
		case 'b':
			commands_count++;
			commands = realloc(commands, commands_count);
			commands[commands_count - 1] = (uint8_t)strtol(optarg, NULL, 0);
			break;
		case 'C':
			clear = 1;
			break;
		case 'M':
			home = 1;
			break;
		case 'c':
			cursor = atoi(optarg);
			if (cursor < 0 || cursor > 3) {
				fprintf(stderr, "invalid cursor\n");
				p_exit(1);
			}
			break;
		case 't':
			text = strdup(optarg);
			break;
		case 'l':
			line = atoi(optarg);
			if (line < 0 || line > 3) {
				fprintf(stderr, "invalid line\n");
				p_exit(1);
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

int main(int argc, char *argv[])
{
	int err = 0, i;

	/* parse command line options */
	if (p_options(argc, argv)) {
		fprintf(stderr, "invalid command line options\n");
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

	/* initialize hd44780 */
	hd44780 = ftdi_hd44780_init(device, reset, d4, d5, d6, d7, en, rw, rs);
	if (!hd44780) {
		fprintf(stderr, "ftdi_hd44780_init() failed\n");
		return -1;
	}

	/* run commands */
	for (i = 0; i < commands_count; i++) {
		ftdi_hd44780_cmd(hd44780, commands[i]);
	}
	if (clear) {
		ftdi_hd44780_cmd(hd44780, 0x01);
	}
	if (home) {
		ftdi_hd44780_cmd(hd44780, 0x02);
	}
	if (cursor >= 0) {
		ftdi_hd44780_cmd(hd44780, 0x0c | cursor);
	}
	if (line >= 0 && line <= 3) {
		ftdi_hd44780_goto_xy(hd44780, 0, line);
	}
	if (text) {
		ftdi_hd44780_write_str(hd44780, text);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

