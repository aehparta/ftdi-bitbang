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
	"  init                        erase and initialize eeprom with defaults\n" \
	"  decode                      read eeprom and print decoded information\n" \
	"                              note: decode after init will fail\n" \
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


int eeprom_changed = 0;

void apply_strings(const char *manufacturer, const char *description, const char *serial)
{
	struct ftdi_context *ftdi = ftdic_get_context();

	/* stupid static buffers, don't really care in this case */
	char m[1024], d[1024], s[1024];
	memset(m, 0, 1024);
	memset(d, 0, 1024);
	memset(s, 0, 1024);
	ftdi_eeprom_get_strings(ftdi, m, 1024, d, 1024, s, 1024);

	/* set which? */
	if (manufacturer) {
		strcpy(m, manufacturer);
	}
	if (description) {
		strcpy(d, description);
	}
	if (serial) {
		strcpy(s, serial);
		int len = 0, i;
		char max = 0;

		if (opt_get_int('W') > 0) {
			len = opt_get_int('W');
			max = 'Z';
		} else if (opt_get_int('X') > 0) {
			len = opt_get_int('X');
			max = 'F';
		} else if (opt_get_int('C') > 0) {
			len = opt_get_int('C');
			max = 0;
		}

		/* generate random padding */
		for (i = strlen(s); i < len; i++) {
			do {
				s[i] = rand() & 0x7f;
			} while ((s[i] < '0' || s[i] > '9') && (s[i] < 'A' || s[i] > max));
		}
		s[i] = '\0';
	}

	/* save */
	ftdi_eeprom_set_strings(ftdi, m, d, s);
}

int apply_command(const char *command, const char *value)
{
	struct ftdi_context *ftdi = ftdic_get_context();

	/* read and decode eeprom */
	if (strcmp(command, "init") == 0 && !value) {
		/* initialize eeprom to defaults */
		if (ftdi_eeprom_initdefaults(ftdi, NULL, NULL, NULL)) {
			fprintf(stderr, "failed to initialize eeprom to defaults: %s\n", ftdi_get_error_string(ftdi));
			return -1;
		}
		eeprom_changed = 1;
	} else if (strcmp(command, "decode") == 0 && !value) {
		if (ftdi_eeprom_decode(ftdi, 1)) {
			fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
			return EXIT_FAILURE;
		}
	} else if (strcmp(command, "manufacturer") == 0 && value) {
		apply_strings(value, NULL, NULL);
		eeprom_changed = 1;
	} else if (strcmp(command, "description") == 0 && value) {
		apply_strings(NULL, value, NULL);
		eeprom_changed = 1;
	} else if (strcmp(command, "serial") == 0 && value) {
		apply_strings(NULL, NULL, value);
		eeprom_changed = 1;
	} else if (strcmp(command, "bus-power") == 0 && value) {
		int p = atoi(value);
		if (p < 100 || p > 500) {
			fprintf(stderr, "invalid bus-power argument value: %d\n", p);
			return -1;
		}
		ftdi_set_eeprom_value(ftdi, MAX_POWER, p);
		eeprom_changed = 1;
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
	if (opt_init(opt_all, "h VPDSURL WXCF", USAGE, HELP) || opt_parse(argc, argv)) {
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
	// return EXIT_FAILURE;
	struct ftdi_context *ftdi = ftdic_get_context();

	/* always read eeprom first */
	if (ftdi_read_eeprom(ftdi)) {
		fprintf(stderr, "failed to read eeprom: %s\n", ftdi_get_error_string(ftdi));
		return EXIT_FAILURE;
	}
	if (ftdi_eeprom_decode(ftdi, 0) && !opt_used('F')) {
		fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
		fprintf(stderr, "run this command using -F to skip this error and use init to reset eeprom contents\n");
		return EXIT_FAILURE;
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

	/* write eeprom if changes were made */
	if (eeprom_changed) {
		if (ftdi_erase_eeprom(ftdi)) {
			fprintf(stderr, "failed to erase eeprom: %s\n", ftdi_get_error_string(ftdi));
			return -1;
		}
		if (ftdi_eeprom_build(ftdi) < 0) {
			fprintf(stderr, "failed to build eeprom: %s\n", ftdi_get_error_string(ftdi));
			return EXIT_FAILURE;
		}
		if (ftdi_write_eeprom(ftdi)) {
			fprintf(stderr, "failed to write eeprom: %s\n", ftdi_get_error_string(ftdi));
			return EXIT_FAILURE;
		}
	}

	/* exit with ok! */
	return EXIT_SUCCESS;
}
