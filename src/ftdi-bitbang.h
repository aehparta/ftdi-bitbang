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
	int d4;
	int d5;
	int d6;
	int d7;
	int en;
	int rw;
	int rs;
	uint8_t l_value;
	uint8_t l_changed;
	uint8_t l_io;
	uint8_t h_value;
	uint8_t h_changed;
	uint8_t h_io;
	int line_width;
	int line_chars_written;
};

struct ftdi_bitbang_dev *ftdi_bitbang_init(struct ftdi_context *ftdi, int reset, int d4, int d5, int d6, int d7, int en, int rw, int rs);
struct ftdi_bitbang_dev * ftdi_bitbang_init_simple(struct ftdi_context *ftdi);
void ftdi_bitbang_free(struct ftdi_bitbang_dev *dev);

int ftdi_bitbang_cmd(struct ftdi_bitbang_dev *dev, uint8_t command);
int ftdi_bitbang_write_data(struct ftdi_bitbang_dev *dev, uint8_t data);
int ftdi_bitbang_write_str(struct ftdi_bitbang_dev *dev, char *str);

int ftdi_bitbang_set_pin(struct ftdi_bitbang_dev *dev, int bit, int value);
int ftdi_bitbang_set_io(struct ftdi_bitbang_dev *dev, int bit, int io);

int ftdi_bitbang_goto_xy(struct ftdi_bitbang_dev *dev, int x, int y);

int ftdi_bitbang_set_line_width(struct ftdi_bitbang_dev *dev, int line_width);


#endif /* __FTDI_bitbang_H__ */
