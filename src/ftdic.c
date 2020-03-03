/*
 * Common FTDI handling.
 */

#include <stdio.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include "opt.h"


struct ftdi_context *ftdi = NULL;


int ftdic_init(void)
{
	int err = 0;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return -1;
	}
	err = ftdi_set_interface(ftdi, opt_get_int('I'));
	if (err < 0) {
		fprintf(stderr, "unable to set selected interface on ftdi device: %d (%s)\n", err, ftdi_get_error_string(ftdi));
		return -1;
	}

	/* find first device if vid or pid is zero */
	// n = ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid);
	// if (n < 1) {
	// 	fprintf(stderr, "unable to find any matching device\n");
	// 	return -1;
	// }
	// match = list;
	// if (usb_description || usb_serial || usb_id) {
	// 	for (i = 0; i < n; i++) {
	// 		char m[1024], d[1024], s[1024];
	// 		memset(m, 0, 1024);
	// 		memset(d, 0, 1024);
	// 		memset(s, 0, 1024);
	// 		ftdi_usb_get_strings(ftdi, list->dev, m, 1024, d, 1024, s, 1024);
	// 		if (usb_description) {
	// 			if (strcmp(usb_description, d) != 0) {
	// 				match = match->next;
	// 				continue;
	// 			}
	// 		}
	// 		if (usb_serial) {
	// 			if (strcmp(usb_serial, s) != 0) {
	// 				match = match->next;
	// 				continue;
	// 			}
	// 		}
	// 		if (!usbid_is_match(match->dev)) {
	// 			match = match->next;
	// 			continue;
	// 		}
	// 		break;
	// 	}
	// 	if (i >= n) {
	// 		fprintf(stderr, "unable to find any matching device\n");
	// 		ftdi_list_free(&list);
	// 		return -1;
	// 	}
	// }
	// err = ftdi_usb_open_dev(ftdi, match->dev);
	// ftdi_list_free(&list);
	// if (err < 0) {
	// 	fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
	// 	return -1;
	// }

	/* reset chip */
	if (opt_used('R')) {
		if (ftdi_usb_reset(ftdi)) {
			fprintf(stderr, "failed to reset device: %s\n", ftdi_get_error_string(ftdi));
			return -1;
		}
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_RESET);
	}

	/* set bitmode */
	if (strcmp(opt_get('M'), "mpsse") == 0) {
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);
	} else {
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG);
	}

	return 0;
}

int ftdic_list(void)
{
	return 0;
}
