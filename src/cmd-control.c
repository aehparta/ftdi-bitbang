/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = "hV:P:D:S:I:RENom:d:s:";
struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "vid", required_argument, NULL, 'V' },
	{ "pid", required_argument, NULL, 'P' },
	{ "description", required_argument, NULL, 'D' },
	{ "serial", required_argument, NULL, 'S' },
	{ "interface", required_argument, NULL, 'I' },
	{ "reset", no_argument, NULL, 'R' },
	{ "eeprom-erase", no_argument, NULL, 'E' },
	{ "eeprom-initialize", no_argument, NULL, 'N' },
	{ "eeprom-decode", no_argument, NULL, 'o' },
	{ "set-manufacturer", required_argument, NULL, 'm' },
	{ "set-description", required_argument, NULL, 'd' },
	{ "set-serial", required_argument, NULL, 's' },
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
/* ftdi device context */
struct ftdi_context *ftdi = NULL;

/* if device should be reset at start */
int reset = 0;
/* if eeprom should be erased first */
int ee_erase = 0;
/* initialize eeprom to default values, erases eeprom */
int ee_initialize = 0;
/* if eeprom should be decoded */
int ee_decode = 0;
/* if eeprom should be read first */
int ee_rd = 0;
/* if eeprom should be written after */
int ee_wr = 0;

char *set_description = NULL;
char *set_serial = NULL;
char *set_manufacturer = NULL;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
	if (set_description) {
		free(set_description);
	}
	if (set_serial) {
		free(set_serial);
	}
	if (set_manufacturer) {
		free(set_manufacturer);
	}
	if (ftdi) {
		ftdi_free(ftdi);
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
	int i;

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
		case 'E':
			ee_erase = 1;
			break;
		case 'N':
			ee_erase = 1;
			ee_initialize = 1;
			ee_wr = 1;
			break;
		case 'o':
			ee_decode = 1;
			ee_rd = 1;
			break;
		case 'm':
			set_manufacturer = strdup(optarg);
			ee_rd = 1;
			ee_wr = 1;
			break;
		case 'd':
			set_description = strdup(optarg);
			ee_rd = 1;
			ee_wr = 1;
			break;
		case 's':
			set_serial = strdup(optarg);
			ee_rd = 1;
			ee_wr = 1;
			break;
		default:
		case '?':
		case 'h':
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
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = cmd_init(usb_vid, usb_pid, usb_description, usb_serial, interface);
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* reset chip */
	if (reset) {
		if (ftdi_usb_reset(ftdi)) {
			fprintf(stderr, "failed to reset device: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}
	/* erase eeprom */
	if (ee_erase) {
		if (ftdi_erase_eeprom(ftdi)) {
			fprintf(stderr, "failed to erase eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}
	/* read eeprom if needed */
	if (ee_rd) {
		if (ftdi_read_eeprom(ftdi) && !ee_initialize) {
			fprintf(stderr, "failed to read eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
		if (ftdi_eeprom_decode(ftdi, 0) && !ee_initialize) {
			fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}
	/* initialize eeprom to defaults */
	if (ee_initialize) {
		if (ftdi_eeprom_initdefaults(ftdi, "", "", "")) {
			fprintf(stderr, "failed to init defaults to eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}
	/* set strings to eeprom */
	if (set_manufacturer || set_description || set_serial) {
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(libusb_get_device(ftdi->usb_dev), &desc);
		if (!set_manufacturer) {
			set_manufacturer = calloc(1, 128);
			libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iManufacturer, set_manufacturer, 127);
		}
		if (!set_description) {
			set_description = calloc(1, 128);
			libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iProduct, set_description, 127);
		}
		if (!set_serial) {
			set_serial = calloc(1, 128);
			libusb_get_string_descriptor_ascii(ftdi->usb_dev, desc.iSerialNumber, set_serial, 127);
		}
		ftdi_eeprom_set_strings(ftdi, set_manufacturer, set_description, set_serial);
	}

	/* decode eeprom */
	if (ee_decode) {
		if (ftdi_eeprom_decode(ftdi, 1)) {
			fprintf(stderr, "failed to decode eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}

	/* write eeprom data */
	if (ee_wr) {
		int n = ftdi_eeprom_build(ftdi);
		if (n < 0) {
			fprintf(stderr, "failed to build eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
		if (ftdi_write_eeprom(ftdi)) {
			fprintf(stderr, "failed to write eeprom: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

