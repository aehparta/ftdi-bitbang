/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef __FTDI_bitbang_H__
#define __FTDI_bitbang_H__

#include <stdlib.h>
#include <ftdi.h>

struct ftdi_bitbang_dev {
	struct ftdi_context *ftdi;
	uint8_t l_value;
	uint8_t l_changed;
	uint8_t l_io;
	uint8_t h_value;
	uint8_t h_changed;
	uint8_t h_io;
};

struct ftdi_bitbang_dev *ftdi_bitbang_init(struct ftdi_context *ftdi);
void ftdi_bitbang_free(struct ftdi_bitbang_dev *dev);

int ftdi_bitbang_set_pin(struct ftdi_bitbang_dev *dev, int bit, int value);
int ftdi_bitbang_set_io(struct ftdi_bitbang_dev *dev, int bit, int io);

int ftdi_bitbang_write(struct ftdi_bitbang_dev *dev);

int ftdi_bitbang_read(struct ftdi_bitbang_dev *dev);


#endif /* __FTDI_bitbang_H__ */
