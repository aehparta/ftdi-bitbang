/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef __FTDI_bitbang_H__
#define __FTDI_bitbang_H__

#include <stdlib.h>
#include <libftdi1/ftdi.h>

struct ftdi_bitbang_state {
	uint8_t l_value;
	uint8_t l_changed;
	uint8_t l_io;
	uint8_t h_value;
	uint8_t h_changed;
	uint8_t h_io;
	/* BITMODE_BITBANG or BITMODE_MPSSE */
	int mode;
};
struct ftdi_bitbang_context {
	struct ftdi_context *ftdi;
	struct ftdi_bitbang_state state;
};

struct ftdi_bitbang_context *ftdi_bitbang_init(struct ftdi_context *ftdi, int mode, int load_state);
void ftdi_bitbang_free(struct ftdi_bitbang_context *dev);

int ftdi_bitbang_set_pin(struct ftdi_bitbang_context *dev, int bit, int value);
int ftdi_bitbang_set_io(struct ftdi_bitbang_context *dev, int bit, int io);

int ftdi_bitbang_write(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_low(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_high(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_read_pin(struct ftdi_bitbang_context *dev, uint8_t pin);

int ftdi_bitbang_load_state(struct ftdi_bitbang_context *dev);
int ftdi_bitbang_save_state(struct ftdi_bitbang_context *dev);


#endif /* __FTDI_bitbang_H__ */
