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

int _data_out(struct ftdi_bitbang_dev *dev)
{
	char buf[6];
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

int _write_nibble(struct ftdi_bitbang_dev *dev, int rs, uint8_t data)
{
	ftdi_bitbang_set_io(dev, dev->d4, 1);
	ftdi_bitbang_set_io(dev, dev->d5, 1);
	ftdi_bitbang_set_io(dev, dev->d6, 1);
	ftdi_bitbang_set_io(dev, dev->d7, 1);

	ftdi_bitbang_set_pin(dev, dev->d4, data & 0x1);
	ftdi_bitbang_set_pin(dev, dev->d5, data & 0x2);
	ftdi_bitbang_set_pin(dev, dev->d6, data & 0x4);
	ftdi_bitbang_set_pin(dev, dev->d7, data & 0x8);
	ftdi_bitbang_set_pin(dev, dev->en, 1);
	ftdi_bitbang_set_pin(dev, dev->rw, 0);
	ftdi_bitbang_set_pin(dev, dev->rs, rs);
	_data_out(dev);

	ftdi_bitbang_set_pin(dev, dev->en, 0);
	_data_out(dev);

	return 0;
}

struct ftdi_bitbang_dev *ftdi_bitbang_init(struct ftdi_context *ftdi, int reset, int d4, int d5, int d6, int d7, int en, int rw, int rs)
{
	struct ftdi_bitbang_dev *dev = malloc(sizeof(struct ftdi_bitbang_dev));
	if (!dev) {
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	/* save args */
	dev->ftdi = ftdi;
	dev->d4 = d4;
	dev->d5 = d5;
	dev->d6 = d6;
	dev->d7 = d7;
	dev->en = en;
	dev->rw = rw;
	dev->rs = rs;

	/* set bitmode to mpsse */
	ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);

	/* setup io pins as outputs */
	ftdi_bitbang_set_io(dev, dev->d4, 1);
	ftdi_bitbang_set_io(dev, dev->d5, 1);
	ftdi_bitbang_set_io(dev, dev->d6, 1);
	ftdi_bitbang_set_io(dev, dev->d7, 1);
	ftdi_bitbang_set_io(dev, dev->en, 1);
	ftdi_bitbang_set_io(dev, dev->rw, 1);
	ftdi_bitbang_set_io(dev, dev->rs, 1);

	/* reset bitbang so that it will be in 4 bit state for sure */
	if (reset) {
		_write_nibble(dev, 0, 0x3);
		_os_sleep(5e-3);
		_write_nibble(dev, 0, 0x3);
		_os_sleep(5e-3);
		_write_nibble(dev, 0, 0x3);
		_os_sleep(5e-3);
		_write_nibble(dev, 0, 0x2);
		_os_sleep(5e-3);
		/* clear display, set cursor home */
		ftdi_bitbang_cmd(dev, 0x01);
		/* entry mode: move cursor right */
		ftdi_bitbang_cmd(dev, 0x06);
		/* display on */
		ftdi_bitbang_cmd(dev, 0x0c);
		/* cursor/shift */
		ftdi_bitbang_cmd(dev, 0x10);
	}

	return dev;
}

struct ftdi_bitbang_dev * ftdi_bitbang_init_simple(struct ftdi_context *ftdi)
{
	return ftdi_bitbang_init(ftdi, 1, 0, 1, 2, 3, 4, 5, 6);
}

void ftdi_bitbang_free(struct ftdi_bitbang_dev *dev)
{
	free(dev);
}

int ftdi_bitbang_cmd(struct ftdi_bitbang_dev *dev, uint8_t command)
{
	_write_nibble(dev, 0, command >> 4);
	_write_nibble(dev, 0, command);
	_os_sleep(2e-3);
	return 0;
}

int ftdi_bitbang_write_data(struct ftdi_bitbang_dev *dev, uint8_t data)
{
	_write_nibble(dev, 1, data >> 4);
	_write_nibble(dev, 1, data);
	_os_sleep(37e-6);
	return 0;
}

int ftdi_bitbang_write_char(struct ftdi_bitbang_dev *dev, char ch)
{
	// if (ch == '\n') {
	// 	return 0;
	// }
	// if (dev->line_width > 0 && dev->line_chars_written >= dev->line_width) {
	// 	return -1;
	// }
	ftdi_bitbang_write_data(dev, (uint8_t)ch);
	// dev->line_chars_written++;
	return 0;
}

int ftdi_bitbang_write_str(struct ftdi_bitbang_dev *dev, char *str)
{
	for (int i = 0; i < strlen(str); i++) {
		ftdi_bitbang_write_char(dev, str[i]);
	}
	return 0;
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

// uint8_t ftdi_bitbang_read_byte(struct ftdi_bitbang_dev *dev, int rs)
// {
// 	uint8_t b = 0;
// 	ftdi_bitbang_set_io(dev, dev->d4, 0);
// 	ftdi_bitbang_set_io(dev, dev->d5, 0);
// 	ftdi_bitbang_set_io(dev, dev->d6, 0);
// 	ftdi_bitbang_set_io(dev, dev->d7, 0);

// 	ftdi_bitbang_set_pin(dev, dev->en, 1);
// 	ftdi_bitbang_set_pin(dev, dev->rw, 1);
// 	ftdi_bitbang_set_pin(dev, dev->rs, rs);
// 	_data_out(dev);

// 	ftdi_read_data();

// 	ftdi_bitbang_set_pin(dev, dev->en, 0);
// 	_data_out(dev);

// 	ftdi_bitbang_set_pin(dev, dev->d4, 0);
// 	ftdi_bitbang_set_pin(dev, dev->d5, 0);
// 	ftdi_bitbang_set_pin(dev, dev->d6, 0);
// 	ftdi_bitbang_set_pin(dev, dev->d7, 0);
// 	ftdi_bitbang_set_pin(dev, dev->en, 0);
// 	_data_out(dev);
// }

int ftdi_bitbang_goto_xy(struct ftdi_bitbang_dev *dev, int x, int y)
{
	if (x < 0 || x > 39 || y < 0 || y > 3) {
		return -1;
	}
	ftdi_bitbang_cmd(dev, 0x80 | (y * 40 + x));
	return 0;
}

int ftdi_bitbang_set_line_width(struct ftdi_bitbang_dev *dev, int line_width)
{
	dev->line_width = line_width;
	return 0;
}
