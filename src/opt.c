
#include <getopt.h>


static struct option opt_all = {
	{ "help", no_argument, NULL, 'h' },
	{ "vid", required_argument, NULL, 'V' },
	{ "pid", required_argument, NULL, 'P' },
	{ "description", required_argument, NULL, 'D' },
	{ "serial", required_argument, NULL, 'S' },
	{ "interface", required_argument, NULL, 'I' },
	{ "usbid", required_argument, NULL, 'U' },
	{ "reset", no_argument, NULL, 'R' },
	{ "list", no_argument, NULL, 'L' },
	{ 0, 0, 0, 0 }
};

