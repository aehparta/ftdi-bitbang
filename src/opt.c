/*
 * Option parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "opt.h"


struct opt_option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
	char *description;
};

struct opt_option opt_all[] = {
	{
		"help", no_argument, 0, 'h',
		"display this help and exit"
	},
	{
		"vid", required_argument, 0, 'V',
		"usb vendor id"
	},
	{
		"pid", required_argument, 0, 'P',
		"usb product id\n"
		"as default vid and pid are zero, so any first compatible ftdi device is used"
	},
	{
		"description", required_argument, 0, 'D',
		"usb description (product) to use for opening right device, default none"
	},
	{
		"serial", required_argument, 0, 'S',
		"usb serial to use for opening right device, default none"
	},
	{
		"interface", required_argument, 0, 'I',
		"ftx232 interface number, defaults to first"
	},
	{
		"usbid", required_argument, 0, 'U',
		"usbid to use for opening right device (sysfs format, e.g. 1-2.3), default none"
	},
	{
		"reset", no_argument, 0, 'R',
		"do usb reset on the device at start"
	},
	{
		"list", no_argument, 0, 'L',
		"list devices that can be found with given parameters"
	},
	{ 0, 0, 0, 0, 0 }
};


char *opt_used = "hVP";


void opt_help(void)
{
	for (int i = 0; opt_all[i].name; i++) {
		int l = 5;
		if (!strchr(opt_used, opt_all[i].val)) {
			continue;
		}

		printf("  -%c, --%-21s ", opt_all[i].val, opt_all[i].name);

		/* print description */
		char *orig = strdup(opt_all[i].description);
		char *p = orig;
		for (int pad = 0; p; pad = 30) {
			printf("%*s%s\n", pad, "", strsep(&p, "\n"));
		}
		free(orig);
	}
}
