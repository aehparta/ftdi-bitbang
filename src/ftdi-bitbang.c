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
#include "ftdi-bitbang.h"

static long double _os_time()
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (long double)((long double)tp.tv_sec + (long double)tp.tv_nsec / 1e9);
}

static void _os_sleep(long double t)
{
	struct timespec tp;
	long double integral;
	t += _os_time();
	tp.tv_nsec = (long)(modfl(t, &integral) * 1e9);
	tp.tv_sec = (time_t)integral;
	while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) == EINTR);
}

struct ftdi_bitbang_dev *ftdi_bitbang_init(struct ftdi_context *ftdi)
{
	struct ftdi_bitbang_dev *dev = malloc(sizeof(struct ftdi_bitbang_dev));
	if (!dev) {
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));
	dev->l_changed = 0xff;
	dev->h_changed = 0xff;

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
	uint8_t *v = bit < 8 ? &dev->l_value : &dev->h_value;
	uint8_t *c = bit < 8 ? &dev->l_changed : &dev->h_changed;
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
	uint8_t *v = bit < 8 ? &dev->l_io : &dev->h_io;
	uint8_t *c = bit < 8 ? &dev->l_changed : &dev->h_changed;
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
	if (dev->l_changed) {
		buf[n++] = 0x80;
		buf[n++] = dev->l_value;
		buf[n++] = dev->l_io;
		dev->l_changed = 0;
	}
	if (dev->h_changed) {
		buf[n++] = 0x82;
		buf[n++] = dev->h_value;
		buf[n++] = dev->h_io;
		dev->h_changed = 0;
	}
	ftdi_write_data(dev->ftdi, buf, n);
	return 0;
}

int ftdi_bitbang_read(struct ftdi_bitbang_dev *dev)
{
	uint8_t buf[2] = { 0x81, 0x83 };
	ftdi_write_data(dev->ftdi, &buf[0], 1);
	ftdi_read_data(dev->ftdi, &buf[0], 1);
	ftdi_write_data(dev->ftdi, &buf[1], 1);
	ftdi_read_data(dev->ftdi, &buf[1], 1);
	return (int)(buf[0] | (buf[1] << 8));
}
