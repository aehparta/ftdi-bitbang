/*
 * Option parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "opt.h"

enum {
	OPT_FILTER_NONE,
	OPT_FILTER_INT,
	OPT_FILTER_NUM,
	OPT_FILTER_HEX
};

struct opt_filter {
	int type;
	double min;
	double max;
};

struct opt_option {
	int short_name;
	const char *name;
	int has_arg;
	int used; /* zero or less when not used (-1 when default set and value should be freed) */
	char *value;
	int (*callback)(int short_name, char *value);
	const char *description;
	struct opt_filter filter;
};

struct opt_option opt_all[] = {
	{ 'h', "help", no_argument, 0, NULL, NULL, "display this help and exit", { 0 } },
	{
		'V', "vid", required_argument, 0, "0", NULL, "usb vendor id",
		{ OPT_FILTER_HEX, 0x0000, 0xffff }
	},
	{
		'P', "pid", required_argument, 0, "0", NULL,
		"usb product id\n"
		"when vid and pid are zero, any first ftdi device found is used",
		{ OPT_FILTER_HEX, 0x0000, 0xffff }
	},
	{ 'D', "description", required_argument, 0, NULL, NULL, "usb description (product) to use for opening right device", { 0 } },
	{ 'S', "serial", required_argument, 0, NULL, NULL, "usb serial to use for opening right device", { 0 } },
	{ 'I', "interface", required_argument, 0, "1", NULL, "ftx232 interface number, defaults to first", { 0 } },
	{ 'U', "usbid", required_argument, 0, NULL, NULL, "usbid to use for opening right device (sysfs format, e.g. 1-2.3)", { 0 } },
	{ 'R', "reset", no_argument, 0, NULL, NULL, "do usb reset on the device at start", { 0 } },
	{ 'L', "list", no_argument, 0, NULL, NULL, "list devices that can be found with given parameters", { 0 } },
	{ 'M', "mode", required_argument, 0, "bitbang", NULL, "set device bitmode, use 'bitbang' or 'mpsse'", { 0 } },
	{ 'B', "baudrate", required_argument, 0, NULL, NULL, "set device baudrate, will default to what ever the device is using", { OPT_FILTER_INT, 0, 20e6 } },

	/* ftdi-control only */
	{ 'E', "ee-erase", no_argument, 0, NULL, NULL, "erase eeprom, sometimes needed if eeprom has already been initialized", { 0 } },
	{ 'N', "ee-init", no_argument, 0, NULL, NULL, "erase and initialize eeprom with defaults", { 0 } },
	{ 'O', "ee-decode", no_argument, 0, NULL, NULL, "read eeprom and print decoded information", { 0 } },
	{ 'G', "ee-manufacturer", required_argument, 0, NULL, NULL, "write manufacturer string", { 0 } },
	{ 'T', "ee-description", required_argument, 0, NULL, NULL, "write description (product) string", { 0 } },
	{ 'Z', "ee-serial", required_argument, 0, NULL, NULL, "write serial string", { 0 } },
	{ 'W', "ee-serial-len", required_argument, 0, NULL, NULL, "pad serial with randomized ascii letters and numbers to this length (upper case)", { 0 } },
	{ 'X', "ee-serial-hex", required_argument, 0, NULL, NULL, "pad serial with randomized hex to this length (upper case)", { 0 } },
	{ 'C', "ee-serial-dec", required_argument, 0, NULL, NULL, "pad serial with randomized numbers to this length", { 0 } },
	{ 'A', "ee-bus-power", required_argument, 0, NULL, NULL, "bus power drawn by the device (100-500 mA)", { 0 } },

	/* ftdi-hd44780 only */
	{ '4', "d4", required_argument, 0, NULL, NULL, "data pin 4, default pin is 0", { OPT_FILTER_INT, 0, 15 } },
	{ '5', "d5", required_argument, 0, NULL, NULL, "data pin 5, default pin is 1", { OPT_FILTER_INT, 0, 15 } },
	{ '6', "d6", required_argument, 0, NULL, NULL, "data pin 6, default pin is 2", { OPT_FILTER_INT, 0, 15 } },
	{ '7', "d7", required_argument, 0, NULL, NULL, "data pin 7, default pin is 3", { OPT_FILTER_INT, 0, 15 } },
	{ 'e', "en", required_argument, 0, NULL, NULL, "enable pin, default pin is 4", { OPT_FILTER_INT, 0, 15 } },
	{ 'r', "rw", required_argument, 0, NULL, NULL, "read/write pin, default pin is 5", { OPT_FILTER_INT, 0, 15 } },
	{ 's', "rs", required_argument, 0, NULL, NULL, "register select pin, default pin is 6", { OPT_FILTER_INT, 0, 15 } },

	{ 0, 0, 0, 0, 0, 0, 0, { 0 } }
};


char *opts_in_use = NULL;

int opt_init(char *use)
{
	opts_in_use = use ? strdup(use) : NULL;

	/* for checking overlapping options */
#ifdef DEBUG
	for (int i = 0; opt_all[i].name; i++) {
		for (int j = 0; opt_all[j].name; j++) {
			if (opt_all[i].short_name == opt_all[j].short_name && i != j) {
				printf("overlapping short option: %c\n", opt_all[i].short_name);
			}
		}
	}
#endif

	return 0;
}

void opt_quit(void)
{
	opts_in_use ? free(opts_in_use) : NULL;
	for (int i = 0; opt_all[i].name; i++) {
		if (opt_all[i].used != 0 && opt_all[i].value) {
			free(opt_all[i].value);
		}
	}
}

int opt_set_callback(int short_name, int (*callback)(int short_name, char *value))
{
	for (int i = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}
		if (opt_all[i].short_name == short_name) {
			opt_all[i].callback = callback;
			return 0;
		}
	}
	return -1;
}

int opt_parse_single(struct opt_option * opt)
{
	/* if help was asked, show and exit immediately */
	if (opt->short_name == 'h') {
		opt_help();
		exit(0);
	}

	/* if callback is set */
	if (opt->callback && opt->callback(opt->short_name, optarg)) {
		return -1;
	}

	/* save value */
	if (opt->used != 0 && opt->value) {
		free(opt->value);
	}
	opt->value = optarg ? strdup(optarg) : NULL;
	opt->used = 1;

	return 0;
}

int opt_parse(int argc, char *argv[])
{
	int err = 0;

	/* generate list of available options */
	size_t n = opts_in_use ? strlen(opts_in_use) : sizeof(opt_all) / sizeof(*opt_all) - 1;
	char *shortopts = malloc(n * 2 + 1);
	struct option *longopts = malloc((n + 1) * sizeof(struct option));

	memset(shortopts, 0, n * 2 + 1);
	memset(longopts, 0, (n + 1) * sizeof(struct option));

	for (int i = 0, j = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}

		/* add to short opts */
		shortopts[strlen(shortopts)] = opt_all[i].short_name;
		if (opt_all[i].has_arg == required_argument) {
			strcat(shortopts, ":");
		}

		/* add to long opts */
		longopts[j].name = opt_all[i].name;
		longopts[j].has_arg = opt_all[i].has_arg;
		longopts[j].flag = NULL;
		longopts[j].val = opt_all[i].short_name;
		j++;
	}

	/* parse */
	int index = 0, c;
	while ((c = getopt_long(argc, argv, shortopts, longopts, &index)) > -1) {
		/* unfortunately this loop is required every time */
		for (int i = 0; opt_all[i].name; i++) {
			if (opt_all[i].short_name == c && opt_parse_single(&opt_all[i])) {
				err = -1;
				goto out;
			}
		}
	}

out:
	free(shortopts);
	free(longopts);
	return err;
}

void opt_help(void)
{
	for (int i = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}

		int n = printf("  -%c, --%s%s", opt_all[i].short_name, opt_all[i].name, opt_all[i].has_arg != required_argument ? "" : "=VALUE");
		n < 30 ? printf("%*s", 30 - n, "") : printf("\n");

		/* print description */
		char *orig = strdup(opt_all[i].description);
		char *p = orig;
		for (int pad = n < 30 ? 0 : 30; p; pad = 30) {
			printf("%*s%s\n", pad, "", strsep(&p, "\n"));
		}
		free(orig);

		/* print default if set */
		if (opt_all[i].used <= 0 && opt_all[i].value) {
			printf("%*s(default: %s)\n", 30, "", opt_all[i].value);
		}
	}
}

int opt_used(int short_name)
{
	for (int i = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}
		if (opt_all[i].short_name == short_name) {
			return opt_all[i].used > 0;
		}
	}
	return 0;
}

int opt_set(int short_name, char *value)
{
	for (int i = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}
		if (opt_all[i].short_name == short_name) {
			opt_all[i].value = strdup(value);
			opt_all[i].used = -1;
			return 0;
		}
	}
	return -1;
}

char *opt_get(int short_name)
{
	for (int i = 0; opt_all[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opt_all[i].short_name)) {
			continue;
		}
		if (opt_all[i].short_name == short_name) {
			return opt_all[i].value;
		}
	}
	return NULL;
}
