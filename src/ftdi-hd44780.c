/*
 * ftdi-hd44780
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
#include "ftdi-hd44780.h"

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

static int _write_nibble(struct ftdi_hd44780_context *dev, int rs, uint8_t data)
{
	ftdi_bitbang_set_io(dev->bb, dev->d4, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d5, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d6, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d7, 1);

	ftdi_bitbang_set_pin(dev->bb, dev->d4, data & 0x1);
	ftdi_bitbang_set_pin(dev->bb, dev->d5, data & 0x2);
	ftdi_bitbang_set_pin(dev->bb, dev->d6, data & 0x4);
	ftdi_bitbang_set_pin(dev->bb, dev->d7, data & 0x8);
	ftdi_bitbang_set_pin(dev->bb, dev->en, 1);
	ftdi_bitbang_set_pin(dev->bb, dev->rw, 0);
	ftdi_bitbang_set_pin(dev->bb, dev->rs, rs);
	ftdi_bitbang_write(dev->bb);

	ftdi_bitbang_set_pin(dev->bb, dev->en, 0);
	ftdi_bitbang_write(dev->bb);

	return 0;
}

struct ftdi_hd44780_context *ftdi_hd44780_init(struct ftdi_bitbang_context *bb, int reset, int d4, int d5, int d6, int d7, int en, int rw, int rs)
{
	struct ftdi_hd44780_context *dev = malloc(sizeof(struct ftdi_hd44780_context));
	if (!dev) {
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	/* save args */
	dev->bb = bb;
	dev->d4 = d4;
	dev->d5 = d5;
	dev->d6 = d6;
	dev->d7 = d7;
	dev->en = en;
	dev->rw = rw;
	dev->rs = rs;

	/* setup io pins as outputs */
	ftdi_bitbang_set_io(dev->bb, dev->d4, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d5, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d6, 1);
	ftdi_bitbang_set_io(dev->bb, dev->d7, 1);
	ftdi_bitbang_set_io(dev->bb, dev->en, 1);
	ftdi_bitbang_set_io(dev->bb, dev->rw, 1);
	ftdi_bitbang_set_io(dev->bb, dev->rs, 1);

	/* reset hd44780 so that it will be in 4 bit state for sure */
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
		ftdi_hd44780_cmd(dev, 0x01);
		/* entry mode: move cursor right */
		ftdi_hd44780_cmd(dev, 0x06);
		/* display on */
		ftdi_hd44780_cmd(dev, 0x0c);
		/* cursor/shift */
		ftdi_hd44780_cmd(dev, 0x10);
	}

	return dev;
}

struct ftdi_hd44780_context *ftdi_hd44780_init_simple(struct ftdi_bitbang_context *bb)
{
	return ftdi_hd44780_init(bb, 1, 0, 1, 2, 3, 4, 5, 6);
}

void ftdi_hd44780_free(struct ftdi_hd44780_context *dev)
{
	free(dev);
}

int ftdi_hd44780_cmd(struct ftdi_hd44780_context *dev, uint8_t command)
{
	_write_nibble(dev, 0, command >> 4);
	_write_nibble(dev, 0, command);
	_os_sleep(2e-3);
	return 0;
}

int ftdi_hd44780_write_data(struct ftdi_hd44780_context *dev, uint8_t data)
{
	_write_nibble(dev, 1, data >> 4);
	_write_nibble(dev, 1, data);
	_os_sleep(37e-6);
	return 0;
}

int ftdi_hd44780_write_char(struct ftdi_hd44780_context *dev, char ch)
{
	// if (ch == '\n') {
	// 	return 0;
	// }
	// if (dev->line_width > 0 && dev->line_chars_written >= dev->line_width) {
	// 	return -1;
	// }
	ftdi_hd44780_write_data(dev, (uint8_t)ch);
	// dev->line_chars_written++;
	return 0;
}

int ftdi_hd44780_write_str(struct ftdi_hd44780_context *dev, char *str)
{
	for (int i = 0; i < strlen(str); i++) {
		ftdi_hd44780_write_char(dev, str[i]);
	}
	return 0;
}

int ftdi_hd44780_goto_xy(struct ftdi_hd44780_context *dev, int x, int y)
{
	if (x < 0 || x > 39 || y < 0 || y > 3) {
		return -1;
	}
	ftdi_hd44780_cmd(dev, 0x80 | (y * 40 + x));
	return 0;
}

int ftdi_hd44780_set_line_width(struct ftdi_hd44780_context *dev, int line_width)
{
	dev->line_width = line_width;
	return 0;
}
