/*
 * ftdi-hd44780
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "ftdi-hd44780.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "i4:5:6:7:e:r:s:b:CMc:t:l:";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "init", no_argument, NULL, 'i' },
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

int init = 0;
int d4 = 0;
int d5 = 1;
int d6 = 2;
int d7 = 3;
int en = 4;
int rw = 5;
int rs = 6;

/* ftdi device context */
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

void p_help()
{
	printf(
	    "  -i, --init                 initialize hd44780 lcd, usually needed only once at first\n"
	    "  -4, --d4=PIN               data pin 4, default pin is 0\n"
	    "  -5, --d5=PIN               data pin 5, default pin is 1\n"
	    "  -6, --d6=PIN               data pin 6, default pin is 2\n"
	    "  -7, --d7=PIN               data pin 7, default pin is 3\n"
	    "  -e, --en=PIN               enable pin, default pin is 4\n"
	    "  -r, --rw=PIN               read/write pin, default pin is 5\n"
	    "  -s, --rs=PIN               register select pin, default pin is 6\n"
	    "  -b, --command=BYTE         send raw hd44780 command, decimal or hexadecimal (0x) byte\n"
	    "                             multiple commands can be given, they are run before any later commands described here\n"
	    "  -C, --clear                clear display\n"
	    "  -M, --home                 move cursor home\n"
	    "  -c, --cursor=VALUE         cursor on/off and blink, accepts value between 0-3\n"
	    "  -t, --text=STRING          write given text to display\n"
	    "  -l, --line=VALUE           move cursor to given line, value between 0-3\n"
	    "\n"
	    "Use HD44780 based LCD displays in 4-bit mode through FTDI FTx232 chips with this command.\n"
	    "\n");
}


int p_options(int c, char *optarg)
{
	switch (c) {
	case 'i':
		init = 1;
		return 1;
	case '4':
		d4 = atoi(optarg);
		return 1;
	case '5':
		d5 = atoi(optarg);
		return 1;
	case '6':
		d6 = atoi(optarg);
		return 1;
	case '7':
		d7 = atoi(optarg);
		return 1;
	case 'e':
		en = atoi(optarg);
		return 1;
	case 'r':
		rw = atoi(optarg);
		return 1;
	case 's':
		rs = atoi(optarg);
		return 1;
	case 'b':
		commands_count++;
		commands = realloc(commands, commands_count);
		commands[commands_count - 1] = (uint8_t)strtol(optarg, NULL, 0);
		return 1;
	case 'C':
		clear = 1;
		return 1;
	case 'M':
		home = 1;
		return 1;
	case 'c':
		cursor = atoi(optarg);
		if (cursor < 0 || cursor > 3) {
			fprintf(stderr, "invalid cursor\n");
			p_exit(1);
		}
		return 1;
	case 't':
		text = strdup(optarg);
		return 1;
	case 'l':
		line = atoi(optarg);
		if (line < 0 || line > 3) {
			fprintf(stderr, "invalid line\n");
			p_exit(1);
		}
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0, i;

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
	device = ftdi_bitbang_init(ftdi);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}
	ftdi_bitbang_load_state(device);

	/* initialize hd44780 */
	hd44780 = ftdi_hd44780_init(device, init, d4, d5, d6, d7, en, rw, rs);
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

