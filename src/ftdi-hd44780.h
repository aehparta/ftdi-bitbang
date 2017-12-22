/*
 * ftdi-hd44780
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef __FTDI_HD44780_H__
#define __FTDI_HD44780_H__

#include <stdlib.h>
#include <ftdi.h>
#include "ftdi-bitbang.h"

struct ftdi_hd44780_context {
	struct ftdi_bitbang_context *bb;
	int d4;
	int d5;
	int d6;
	int d7;
	int en;
	int rw;
	int rs;
	int line_width;
	int line_chars_written;
};

struct ftdi_hd44780_context *ftdi_hd44780_init(struct ftdi_bitbang_context *bb, int reset, int d4, int d5, int d6, int d7, int en, int rw, int rs);
struct ftdi_hd44780_context * ftdi_hd44780_init_simple(struct ftdi_bitbang_context *bb);
void ftdi_hd44780_free(struct ftdi_hd44780_context *dev);

int ftdi_hd44780_cmd(struct ftdi_hd44780_context *dev, uint8_t command);
int ftdi_hd44780_write_data(struct ftdi_hd44780_context *dev, uint8_t data);
int ftdi_hd44780_write_char(struct ftdi_hd44780_context *dev, char ch);
int ftdi_hd44780_write_str(struct ftdi_hd44780_context *dev, char *str);

int ftdi_hd44780_goto_xy(struct ftdi_hd44780_context *dev, int x, int y);
int ftdi_hd44780_set_line_width(struct ftdi_hd44780_context *dev, int line_width);


#endif /* __FTDI_HD44780_H__ */
