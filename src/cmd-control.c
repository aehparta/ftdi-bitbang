/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "RENom:d:s:";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "ee-erase", no_argument, NULL, 'E' },
	{ "ee-init", no_argument, NULL, 'N' },
	{ "ee-decode", no_argument, NULL, 'o' },
	{ "ee-manufacturer", required_argument, NULL, 'm' },
	{ "ee-description", required_argument, NULL, 'd' },
	{ "ee-serial", required_argument, NULL, 's' },
	{ 0, 0, 0, 0 },
};

/* ftdi device context */
struct ftdi_context *ftdi = NULL;

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

void p_help()
{
	printf(
	    "  -E, --ee-erase             erase eeprom, sometimes needed if eeprom has already been initialized\n"
	    "  -N, --ee-init              erase and initialize eeprom with defaults\n"
	    "  -o, --ee-decode            read eeprom and print decoded information\n"
	    "  -m, --ee-manufacturer=STRING\n"
	    "                             write manufacturer string\n"
	    "  -d, --ee-description=STRING\n"
	    "                             write description (product) string\n"
	    "  -s, --ee-serial=STRING     write serial string\n"
	    "\n"
	    "Basic control and eeprom routines for FTDI FTx232 chips.\n"
	    "\n");
}

int p_options(int c, char *optarg)
{
	switch (c) {
	case 'E':
		ee_erase = 1;
		return 1;
	case 'N':
		ee_erase = 1;
		ee_initialize = 1;
		ee_wr = 1;
		return 1;
	case 'o':
		ee_decode = 1;
		ee_rd = 1;
		return 1;
	case 'm':
		set_manufacturer = strdup(optarg);
		ee_rd = 1;
		ee_wr = 1;
		return 1;
	case 'd':
		set_description = strdup(optarg);
		ee_rd = 1;
		ee_wr = 1;
		return 1;
	case 's':
		set_serial = strdup(optarg);
		ee_rd = 1;
		ee_wr = 1;
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

