/*
 * Common FTDI handling.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ftdic.h"
#include "opt.h"


/* internal functions */
static char *ftdic_get_usbid(struct libusb_device *dev);


/* global common ftdi context holder, only one device cand one interface can be used at once */
struct ftdi_context *ftdi = NULL;


int ftdic_init(void)
{
	int err = 0, i, n;
	struct ftdi_device_list *list;

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

	/* find devices mathing given vid and pid */
	n = ftdi_usb_find_all(ftdi, &list, opt_get_int('V'), opt_get_int('P'));
	if (n < 1) {
		fprintf(stderr, "unable to find any matching device\n");
		return -1;
	}
	/* clone pointer for walking thourgh the list */
	struct ftdi_device_list *match = list;
	/* get and filter using specifiers */
	char *usb_description = opt_get('D');
	char *usb_serial = opt_get('S');
	char *usb_id = opt_get('U');
	if (usb_description || usb_serial || usb_id) {
		for (i = 0; i < n; i++, match = match->next) {
			/* stupid static buffers, don't really care in this case */
			char m[1024], d[1024], s[1024];
			memset(m, 0, 1024);
			memset(d, 0, 1024);
			memset(s, 0, 1024);
			ftdi_usb_get_strings(ftdi, match->dev, m, 1024, d, 1024, s, 1024);

			if (usb_description && strcmp(usb_description, d) != 0) {
				continue;
			}
			if (usb_serial && strcmp(usb_serial, s) != 0) {
				continue;
			}
			if (usb_id && strcmp(usb_id, ftdic_get_usbid(match->dev)) != 0) {
				continue;
			}

			break;
		}

		if (i >= n) {
			fprintf(stderr, "unable to find any matching device\n");
			ftdi_list_free(&list);
			return -1;
		}
	}
	err = ftdi_usb_open_dev(ftdi, match->dev);
	ftdi_list_free(&list);
	if (err < 0) {
		fprintf(stderr, "unable to open ftdi device: %s\n", ftdi_get_error_string(ftdi));
		return -1;
	}

	/* reset chip */
	if (opt_used('R')) {
		if (ftdi_usb_reset(ftdi)) {
			fprintf(stderr, "failed to reset device: %s\n", ftdi_get_error_string(ftdi));
			return -1;
		}
		ftdi_set_bitmode(ftdi, 0x00, BITMODE_RESET);
	}

	/* set bitmode */
	// if (strcmp(opt_get('M'), "mpsse") == 0) {
	// 	ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);
	// } else {
	// 	ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG);
	// }

	return 0;
}

void ftdic_quit(void)
{
	if (ftdi) {
		ftdi_free(ftdi);
	}
	ftdi = NULL;
}

void ftdic_list(void)
{
	int i, n;
	struct ftdi_context *ftdi = NULL;
	struct ftdi_device_list *list;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return;
	}
	/* don't care for vid or pid here */
	n = ftdi_usb_find_all(ftdi, &list, 0, 0);
	if (n < 1) {
		fprintf(stderr, "unable to find any matching device\n");
		return;
	}

	printf("usb id     serial     description          manufacturer\n");
	for (i = 0; i < n; i++, list = list->next) {
		/* stupid static buffers, don't really care in this case */
		char m[1024], d[1024], s[1024];
		memset(m, 0, 1024);
		memset(d, 0, 1024);
		memset(s, 0, 1024);

		ftdi_usb_get_strings(ftdi, list->dev, m, 1024, d, 1024, s, 1024);
		printf("%-10s %-10s %-20s %-20s\n", ftdic_get_usbid(list->dev), s, d, m);
	}

	ftdi_list_free(&list);
	ftdi_free(ftdi);
}

struct ftdi_context *ftdic_get_context(void)
{
	return ftdi;
}


/* internals */


static char *ftdic_get_usbid(struct libusb_device *dev)
{
	static char id[128];
	char *buf = NULL;
	size_t len = 0;
	int n;
	uint8_t port_numbers[7];
	FILE *fh;

	n = libusb_get_port_numbers(dev, port_numbers, sizeof(port_numbers) / sizeof(port_numbers[0]));
	if (n == LIBUSB_ERROR_OVERFLOW) {
		fprintf(stderr, "device has too many port numbers\n");
		return NULL;
	}

	fh = open_memstream(&buf, &len);
	if (!fh)
		return NULL;

	fprintf(fh, "%d-", libusb_get_bus_number(dev));
	for (int i = 0;;) {
		fprintf(fh, "%d", port_numbers[i]);
		if (++i == n)
			break;
		fputc('.', fh);
	}
	fclose(fh);

	memset(id, 0, sizeof(id));
	strncpy(id, buf, sizeof(id) - 1);
	free(buf);

	return id;
}
