/*
 * ftdi-bitbang
 *
 * Common routines for all command line utilies.
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <libftdi1/ftdi.h>
#include "cmd-common.h"


struct ftdi_context *cmd_init(int usb_vid, int usb_pid, const char *usb_description, const char *usb_serial, int interface)
{
	int err = 0;
	struct ftdi_context *ftdi = NULL;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return NULL;
	}
	err = ftdi_set_interface(ftdi, interface);
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return NULL;
	}
	/* find first device if vid or pid is zero */
	if (usb_vid == 0 && usb_pid == 0) {
		struct ftdi_device_list *list;
		if (ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid) < 1) {
			fprintf(stderr, "unable to find any matching device\n");
			return NULL;
		}
		err = ftdi_usb_open_dev(ftdi, list->dev);
		ftdi_list_free(&list);
		if (err < 0) {
			fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
			return NULL;
		}
	} else {
		err = ftdi_usb_open_desc(ftdi, usb_vid, usb_pid, usb_description, usb_serial);
		if (err < 0) {
			fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
			return NULL;
		}
	}

	return ftdi;
}

