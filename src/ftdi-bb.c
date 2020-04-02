/*
 * FTDI bitbang command.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "ftdic.h"
#include "arg.h"
#include "os.h"
#include "opt-all.h"


#define USAGE "\nUsage: ftdi-bb [options] <commands>"
#define HELP \
	"Commands:\n" \
	"  io=VALUE                    set pins as inputs and outputs, 0 is input, 1 is output (for each bit per pin)\n" \
	"                              value is 16-bit hex, only lower 8-bits are used in bitbang mode\n" \
	"  iod=VALUE                   set pins as inputs and outputs, 0 is input, 1 is output (for each bit per pin)\n" \
	"                              value is 16-bit dec, only lower 8-bits are used in bitbang mode\n" \
	"  oPIN                        set as output\n" \
	"  iPIN                        set as input\n" \
	"  w=VALUE                     write pin states, value is 16-bit hex, only lower 8-bits are used in bitbang mode\n" \
	"  wd=VALUE                    write pin states, value is 16-bit dec, only lower 8-bits are used in bitbang mode\n" \
	"  hPIN                        set pin high and as output\n" \
	"  lPIN                        set pin low and as output\n" \
	"  r                           read pins and print to stdout as hex\n" \
	"  rd                          read pins and print to stdout as dec\n" \
	"  rb                          read pins and print to stdout as binary\n" \
	"  rPIN                        read pin and print to stdout\n" \
	"  d=NUMBER                    delay before executing next command (seconds, float)\n" \
	"  t=NUMBER                    delay until this many seconds has passed compared to the time of\n" \
	"                              first command execution\n" \
	"                              this is uses a busy-while-loop and will consume a lot of cpu to achieve\n" \
	"                              as much accuracy as possible.\n" \
	"  -                           read commands from piped or redirected stdin\n" \
	"\n" \
	"Options are processed before commands.\n" \
	"File read from stdin can also include comments. Comments start with '#' or ';' character and end with line feed.\n"

#define VERBOSE(...) do { if (verbose) { fprintf(stderr, ##__VA_ARGS__); } } while (0)
int verbose = 0;

int pins;


int apply_command(const char *command, const char *value)
{
	static os_time_t t = -1;

	/* save time of first command execution */
	if (t < 0) {
		t = os_timef();
	}

	/* which command is it?!? */
	if (strcmp(command, "io") == 0 && value) {
		/* set io direction from hex string */
		int v = strtoul(value, NULL, 16);
		ftdic_bb_dir_io(v);
		VERBOSE("set io dir hex: %02x\n", v);
	} else if (strcmp(command, "iod") == 0 && value) {
		/* set io direction from dec string */
		int v = atoi(value);
		ftdic_bb_dir_io(v);
		VERBOSE("set io dir dec: %u\n", v);
	} else if (strlen(command) > 1 && command[0] == 'o' && isdigit(command[1])) {
		/* set single pin as output */
		int pin = atoi(&command[1]);
		ftdic_bb_dir_io_pin(pin, 1);
	} else if (strlen(command) > 1 && command[0] == 'i' && isdigit(command[1])) {
		/* set single pin as input */
		int pin = atoi(&command[1]);
		ftdic_bb_dir_io_pin(pin, 0);
	} else if (strcmp(command, "w") == 0 && value) {
		/* write pins from hex string */
		int v = strtoul(value, NULL, 16);
		ftdic_bb_set_pins(v);
		VERBOSE("write pins hex: %02x\n", v);
	} else if (strcmp(command, "wd") == 0 && value) {
		/* write pins from dec string */
		int v = atoi(value);
		ftdic_bb_set_pins(v);
		VERBOSE("write pins dec: %u\n", v);
	} else if (strlen(command) > 1 && command[0] == 'h' && isdigit(command[1])) {
		/* set single pin high */
		int pin = atoi(&command[1]);
		if (!ftdic_bb_set_pin(pin, 1)) {
			VERBOSE("set pin #%u high\n", pin);
		} else {
			fprintf(stderr, "invalid pin high command: %s\n", command);
		}
	} else if (strlen(command) > 1 && command[0] == 'l' && isdigit(command[1])) {
		/* set single pin low */
		int pin = atoi(&command[1]);
		if (!ftdic_bb_set_pin(pin, 0)) {
			VERBOSE("set pin #%u low\n", pin);
		} else {
			fprintf(stderr, "invalid pin low command: %s\n", command);
		}
	} else if (strcmp(command, "r") == 0 && !value) {
		/* read pin states and print as hex */
		int v = ftdic_bb_read();
		if (v < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		opt_used('M') ? printf("%04x\n", v) : printf("%02x\n", v);
	} else if (strcmp(command, "rd") == 0 && !value) {
		/* print dec */
		int v = ftdic_bb_read();
		if (v < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		printf("%u\n", v);
	} else if (strcmp(command, "rb") == 0 && !value) {
		/* print binary */
		int v = ftdic_bb_read();
		if (v < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		for (int i = 0x8000; i > 0; i = i >> 1) {
			printf("%u", v & i ? 1 : 0);
		}
		printf("\n");
	} else if (strlen(command) > 1 && command[0] == 'r' && isdigit(command[1])) {
		int pin = ftdic_bb_read_pin(atoi(&command[1]));
		if (pin < 0) {
			fprintf(stderr, "failed reading pin or invalid pin number\n");
			return -1;
		}
		printf("%u\n", pin);
	} else if (strcmp(command, "d") == 0 && value) {
		os_time_t delay = atof(value);
		VERBOSE("delay for %Lf seconds\n", delay);
		os_sleepf(delay);
	} else if (strcmp(command, "t") == 0 && value) {
		os_time_t until = t + atof(value);
		VERBOSE("delay for %Lf seconds (absolute)\n", until - os_timef());
		while (until > os_timef());
	} else {
		fprintf(stderr, "invalid command and/or value: %s%s%s\n", command, value ? " = " : "", value ? value : "");
	}

	return 0;
}

int main(int argc, char *argv[])
{
	/* free resources on exit */
	atexit(opt_quit);
	atexit(ftdic_quit);

	/* parse options */
	if (opt_init(opt_all, "h VPDSURL IMB v", USAGE, HELP) || opt_parse(argc, argv)) {
		return EXIT_FAILURE;
	}
	if (argc == optind) {
		fprintf(stderr, "no arguments given, nothing to do\n");
		return EXIT_FAILURE;
	}
	verbose = opt_used('v');

	/* initialize ftdi */
	if (ftdic_init()) {
		return EXIT_FAILURE;
	}

	/* read pin states */
	pins = ftdic_bb_read();
	if (pins < 0) {
		fprintf(stderr, "failed reading initial pin states\n");
		return EXIT_FAILURE;
	}

	/* apply arguments */
	struct arg *arg = arg_init(argc - optind, &argv[optind], ",", "=", "-", "#;");
	while (arg_parse(arg) > 0) {
		const char *c = arg_name(arg);
		const char *v = arg_value(arg);
		if (apply_command(c, v)) {
			return EXIT_FAILURE;
		}
		if (!arg_has_sepsa(arg)) {
			ftdic_bb_flush();
		}
	}
	arg_free(arg);

	/* exit with ok! */
	return EXIT_SUCCESS;
}
