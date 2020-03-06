/*
 * FTDI bitbang command.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "ftdic.h"
#include "arg.h"
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
	"  hPIN                        set pin high\n" \
	"  lPIN                        set pin low\n" \
	"  r                           read pins and print to stdout as hex\n" \
	"  rd                          read pins and print to stdout as dec\n" \
	"  rb                          read pins and print to stdout as binary\n" \
	"  rPIN                        read pin and print to stdout\n" \
	"  -                           read commands from piped or redirected stdin\n" \
	"\n" \
	"Options are processed before commands, even those defined after or between commands.\n" \
	"File read from stdin can also include comments. Comments start with '#' or ';' character and end with line feed.\n"


int apply_command(const char *command, const char *value)
{
	struct ftdi_context *ftdi = ftdic_get_context();

	if (strcmp(command, "io") == 0 && value) {
		printf("set io hex\n");
	} else if (strcmp(command, "iod") == 0 && value) {
		printf("set io dec\n");
	} else if (strcmp(command, "w") == 0 && value) {
		printf("write pins hex\n");
	} else if (strcmp(command, "wd") == 0 && value) {
		printf("write pins dec\n");
	} else if (strcmp(command, "r") == 0 && !value) {
		/* print hex */
		int pins = ftdic_bb_read();
		if (pins < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		ftdi->bitbang_mode == BITMODE_MPSSE ? printf("%04x\n", pins) : printf("%02x\n", pins);
	} else if (strcmp(command, "rd") == 0 && !value) {
		/* print dec */
		int pins = ftdic_bb_read();
		if (pins < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		printf("%u\n", pins);
	} else if (strcmp(command, "rb") == 0 && !value) {
		/* print binary */
		int pins = ftdic_bb_read();
		if (pins < 0) {
			fprintf(stderr, "failed reading pins\n");
			return -1;
		}
		for (int i = 0x8000; i > 0; i = i >> 1) {
			printf("%u", pins & i ? 1 : 0);
		}
		printf("\n");
	} else if (strlen(command) > 1 && command[0] == 'r' && isdigit(command[1])) {
		int pin = ftdic_bb_read_pin(atoi(&command[1]));
		if (pin < 0) {
			fprintf(stderr, "failed reading pin or invalid pin number\n");
			return -1;
		}
		printf("%u\n", pin);
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
	if (opt_init(opt_all, "h VPDSURL IMB", USAGE, HELP) || opt_parse(argc, argv)) {
		return EXIT_FAILURE;
	}
	if (argc == optind) {
		fprintf(stderr, "no arguments given, nothing to do\n");
		return EXIT_FAILURE;
	}

	/* initialize ftdi */
	if (ftdic_init()) {
		return EXIT_FAILURE;
	}

	struct ftdi_context *ftdi = ftdic_get_context();
	if (opt_used('M')) {
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);
	} else {
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG);
	}

	/* apply arguments */
	struct arg *arg = arg_init(argc - optind, &argv[optind], NULL, "=", "-", "#;");
	while (arg_parse(arg) > 0) {
		const char *c = arg_name(arg);
		const char *v = arg_value(arg);
		if (apply_command(c, v)) {
			return EXIT_FAILURE;
		}
	}
	arg_free(arg);

	/* exit with ok! */
	return EXIT_SUCCESS;
}
