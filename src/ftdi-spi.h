/*
 * ftdi-spi
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef __FTDI_SPI_H__
#define __FTDI_SPI_H__

#include <stdlib.h>
#include <ftdi.h>
#include "ftdi-bitbang.h"

struct ftdi_spi_context {
	struct ftdi_bitbang_context *bb;
	int sclk;
	int mosi;
	int miso;
	int ss;
	int cpol;
	int cpha;
};

struct ftdi_spi_context *ftdi_spi_init(struct ftdi_bitbang_context *bb, int sclk, int mosi, int miso, int ss, int cpol, int cpha);
void ftdi_spi_free(struct ftdi_spi_context *spi);

int64_t ftdi_spi_transfer(struct ftdi_spi_context *spi, int64_t data_write, uint8_t bit_count);

#endif /* __FTDI_SPI_H__ */
