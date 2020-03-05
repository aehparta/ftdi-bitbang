/*
 * FTDI EEPROM configuration utility.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ftdic.h"
#include "arg.h"
#include "opt-all.h"


#define USAGE "\nUsage: ftdi-cfg [options] <commands>"
#define HELP \
	"Commands:\n" \
	"  erase                       erase eeprom\n" \
	"  init                        erase and initialize eeprom with defaults\n" \
	"  decode                      read eeprom and print decoded information\n" \
	"  manufacturer=STRING         write manufacturer string\n" \
	"  description=STRIGN          write description (product) string\n" \
	"  serial=STRING               write serial string\n" \
	"  bus-power=INT               set bus power drawn by the device (100-500 mA)\n" \
	"  -                           read commands from piped or redirected stdin\n" \
	"\n" \
	"Options are processed before commands, even those defined after or between commands.\n" \
	"File read from stdin can also include comments. Comments start with '#' or ';' character and end with line feed.\n" \
	"\n" \
	"Examples:\n" \
	"  ftdi-cfg -L\n" \
	"  ftdi-cfg --usbid=1-6.1.1 init manufacturer=MeMyself description=MyDevice serial=MYDEV -X 8 bus-power=250\n" \
	"  ftdi-cfg --serial=MYDEV7F3 decode\n" \
	"  ftdi-cfg --usbid=1-6.1.1 init - decode < my-ftdi-cfg-commands.txt\n" \
	"  cat my-ftdi-cfg-commands.txt | ftdi-cfg --usbid=1-6.1.1 init - decode\n" \
	"\n" \
	"Last two examples will first initialize eeprom with defaults, then read commands from given file and last print eeprom contents.\n"



int apply_command(const char *command, const char *value)
{
	struct ftdi_context *ftdi = ftdic_get_context();

	/* read and decode eeprom */
	if (strcmp(command, "decode") == 0 && !value) {
		if (ftdi_read_eeprom(ftdi)) {
			fprintf(stderr, "failed to read eeprom: %s\n", ftdi_get_error_string(ftdi));
			return EXIT_FAILURE;
		}
		if (ftdi_eeprom_decode(ftdi, 1)) {
			fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr, "invalid command and/or value: %s%s%s\n", command, value ? " = " : "", value ? value : "");
	}


	return 0;
}

int main(int argc, char *argv[])
{
	/* random seed */
	srand(time(NULL));

	/* free resources on exit */
	atexit(opt_quit);
	atexit(ftdic_quit);

	/* parse options */
	if (opt_init(opt_all, "h VPDSURL WXC", USAGE, HELP) || opt_parse(argc, argv)) {
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

	/* apply arguments */
	struct arg *arg = arg_init(argc - optind, &argv[optind], ",", "=", "-", "#;");
	while (arg_parse(arg) > 0) {
		const char *c = arg_name(arg);
		const char *v = arg_value(arg);
		if (apply_command(c, v)) {
			return EXIT_FAILURE;
		}
	}

	/* exit with ok! */
	arg_free(arg);
	return EXIT_SUCCESS;
}
