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

/* libusb context holder */
libusb_context *lu_context = NULL;


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
		if (!opt_used('F')) {
			fprintf(stderr, "unable to find any matching device\n");
			return -1;
		} else {
			return ftdi_usb_open(ftdi, 0, 0) ? -1 : 0;
		}
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
	if (lu_context) {
		libusb_exit(lu_context);
	}
	lu_context = NULL;
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

int ftdic_bb_read_low(void)
{
	if (ftdi->bitbang_mode == BITMODE_MPSSE) {
		uint8_t buf[1] = { 0x81 };
		ftdi_usb_purge_rx_buffer(ftdi);
		if (ftdi_write_data(ftdi, &buf[0], 1) != 1) {
			return -1;
		}
		if (ftdi_read_data(ftdi, &buf[0], 1) != 1) {
			return -1;
		}
		return (int)buf[0];
	} else {
		uint8_t pins;
		if (ftdi_read_pins(ftdi, &pins)) {
			return -1;
		}
		return pins;
	}
	return -1;

}

int ftdic_bb_read_high(void)
{
	/* if device is not in MPSSE mode, it only supports pins through 0-7 */
	if (ftdi->bitbang_mode == BITMODE_BITBANG) {
		return -1;
	}

	uint8_t buf[1] = { 0x83 };
	ftdi_usb_purge_rx_buffer(ftdi);
	if (ftdi_write_data(ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	if (ftdi_read_data(ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	return (int)buf[0];
}

int ftdic_bb_read(void)
{
	int h = 0, l = 0;
	l = ftdic_bb_read_low();
	/* if device is not in MPSSE mode, it only supports pins through 0-7 */
	if (ftdi->bitbang_mode == BITMODE_MPSSE) {
		h = ftdic_bb_read_high();
	}
	if (l < 0 || h < 0) {
		return -1;
	}
	return (h << 8) | l;
}

int ftdic_bb_read_pin(uint8_t pin)
{
	if (pin <= 7 && pin >= 0) {
		return (ftdic_bb_read_low() & (1 << pin)) ? 1 : 0;
	} else if (pin >= 8 && pin <= 15) {
		/* if device is not in MPSSE mode, it only supports pins through 0-7 */
		if (ftdi->bitbang_mode == BITMODE_BITBANG) {
			return -1;
		}
		return (ftdic_bb_read_high() & (1 << (pin - 8))) ? 1 : 0;
	}
	return -1;
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
