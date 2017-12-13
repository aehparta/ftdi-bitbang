/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "ftdi-bitbang.h"


struct ftdi_bitbang_dev *ftdi_bitbang_init(struct ftdi_context *ftdi)
{
	struct ftdi_bitbang_dev *dev = malloc(sizeof(struct ftdi_bitbang_dev));
	if (!dev) {
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	/* save args */
	dev->ftdi = ftdi;

	/* set bitmode to mpsse */
	if (ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE) != 0) {
		return NULL;
	}

	return dev;
}

void ftdi_bitbang_free(struct ftdi_bitbang_dev *dev)
{
	free(dev);
}

int ftdi_bitbang_set_pin(struct ftdi_bitbang_dev *dev, int bit, int value)
{
	/* get which byte it is, higher or lower */
	uint8_t *v = bit < 8 ? &dev->state.l_value : &dev->state.h_value;
	uint8_t *c = bit < 8 ? &dev->state.l_changed : &dev->state.h_changed;
	/* get bit in byte mask */
	uint8_t mask = 1 << (bit - (bit < 8 ? 0 : 8));
	/* save old pin state */
	uint8_t was = (*v) & mask;
	/* clear and set new value */
	(*v) = ((*v) & ~mask) | (value ? mask : 0);
	/* set changed if actially changed */
	(*c) |= was != ((*v) & mask) ? mask : 0;
	return 0;
}

int ftdi_bitbang_set_io(struct ftdi_bitbang_dev *dev, int bit, int io)
{
	/* get which byte it is, higher or lower */
	uint8_t *v = bit < 8 ? &dev->state.l_io : &dev->state.h_io;
	uint8_t *c = bit < 8 ? &dev->state.l_changed : &dev->state.h_changed;
	/* get bit in byte mask */
	uint8_t mask = 1 << (bit - (bit < 8 ? 0 : 8));
	/* save old pin state */
	uint8_t was = (*v) & mask;
	/* clear and set new value */
	(*v) = ((*v) & ~mask) | (io ? mask : 0);
	/* set changed */
	(*c) |= was != ((*v) & mask) ? mask : 0;
	return 0;
}

int ftdi_bitbang_write(struct ftdi_bitbang_dev *dev)
{
	uint8_t buf[6];
	int n = 0;
	if (dev->state.l_changed) {
		buf[n++] = 0x80;
		buf[n++] = dev->state.l_value;
		buf[n++] = dev->state.l_io;
		dev->state.l_changed = 0;
	}
	if (dev->state.h_changed) {
		buf[n++] = 0x82;
		buf[n++] = dev->state.h_value;
		buf[n++] = dev->state.h_io;
		dev->state.h_changed = 0;
	}
	ftdi_write_data(dev->ftdi, buf, n);
	return 0;
}

int ftdi_bitbang_read_low(struct ftdi_bitbang_dev *dev)
{
	uint8_t buf[1] = { 0x81 };
	ftdi_usb_purge_rx_buffer(dev->ftdi);
	if (ftdi_write_data(dev->ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	if (ftdi_read_data(dev->ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	return (int)buf[0];
}

int ftdi_bitbang_read_high(struct ftdi_bitbang_dev *dev)
{
	uint8_t buf[1] = { 0x83 };
	ftdi_usb_purge_rx_buffer(dev->ftdi);
	if (ftdi_write_data(dev->ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	if (ftdi_read_data(dev->ftdi, &buf[0], 1) != 1) {
		return -1;
	}
	return (int)buf[0];
}

int ftdi_bitbang_read(struct ftdi_bitbang_dev *dev)
{
	int h, l;
	l = ftdi_bitbang_read_low(dev);
	h = ftdi_bitbang_read_high(dev);
	if (l < 0 || h < 0) {
		return -1;
	}
	return (h << 8) | l;
}

static char *_generate_state_filename(struct ftdi_bitbang_dev *dev)
{
	int i;
	char *state_filename = NULL;
	uint8_t bus, addr, port;
	libusb_device *usb_dev = libusb_get_device(dev->ftdi->usb_dev);
	/* create unique device filename */
	bus = libusb_get_bus_number(usb_dev);
	addr = libusb_get_device_address(usb_dev);
	port = libusb_get_port_number(usb_dev);
	i = asprintf(&state_filename, "%sftdi-bitbang-device-state-%03d.%03d.%03d-%d.%d-%d", _PATH_TMP, bus, addr, port, dev->ftdi->interface, dev->ftdi->type, (unsigned int)getuid());
	if (i < 1 || !state_filename) {
		return NULL;
	}
	return state_filename;
}

int ftdi_bitbang_load_state(struct ftdi_bitbang_dev *dev)
{
	int fd, n;
	char *state_filename;
	struct ftdi_bitbang_state state;
	/* generate filename and open file */
	state_filename = _generate_state_filename(dev);
	if (!state_filename) {
		return -1;
	}
	fd = open(state_filename, O_RDONLY);
	if (fd < 0) {
		free(state_filename);
		return -1;
	}
	/* read contents */
	n = read(fd, &state, sizeof(state));
	if (n == sizeof(state)) {
		memcpy(&dev->state, &state, sizeof(state));
	}
	/* free resources */
	free(state_filename);
	close(fd);
	return 0;
}

int ftdi_bitbang_save_state(struct ftdi_bitbang_dev *dev)
{
	int fd, n;
	char *state_filename;
	/* generate filename and open file */
	state_filename = _generate_state_filename(dev);
	if (!state_filename) {
		return -1;
	}
	fd = open(state_filename, O_WRONLY | O_CREAT);
	if (fd < 0) {
		free(state_filename);
		return -1;
	}
	/* write contents */
	n = write(fd, &dev->state, sizeof(dev->state));
	if (n == sizeof(dev->state)) {
		/* set accessible only by current user */
		fchmod(fd, 0600);
	}
	/* free resources */
	free(state_filename);
	close(fd);
	return 0;
}
