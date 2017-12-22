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
#include "ftdi-spi.h"

struct ftdi_spi_context *ftdi_spi_init(struct ftdi_bitbang_context *bb, int sclk, int mosi, int miso, int ss, int cpol, int cpha)
{
	struct ftdi_spi_context *spi = malloc(sizeof(struct ftdi_spi_context));
	if (!spi) {
		return NULL;
	}
	memset(spi, 0, sizeof(*spi));

	/* save args */
	spi->bb = bb;
	spi->sclk = sclk;
	spi->mosi = mosi;
	spi->miso = miso;
	spi->ss = ss;
	spi->cpol = cpol ? 1 : 0;
	spi->cpha = cpha ? 1 : 0;

	/* setup io pin directions */
	ftdi_bitbang_set_io(spi->bb, spi->sclk, 1);
	ftdi_bitbang_set_io(spi->bb, spi->mosi, 1);
	ftdi_bitbang_set_io(spi->bb, spi->miso, 0);
	ftdi_bitbang_set_io(spi->bb, spi->ss, 1);
	/* setup initial state */
	ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol);
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, 1);
	}
	ftdi_bitbang_write(spi->bb);

	return spi;
}

void ftdi_spi_free(struct ftdi_spi_context *spi)
{
	free(spi);
}

int64_t ftdi_spi_transfer(struct ftdi_spi_context *spi, int64_t data_write, uint8_t bit_count)
{
	int i;
	int64_t data_read = 0;

	/* enable slave */
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, 0);
	}
	ftdi_bitbang_write(spi->bb);

	/* transfer data */
	for (i = bit_count; i > 0; i--) {
		/* write if phase is 0 (default) */
		if (!spi->cpha) {
			ftdi_bitbang_set_pin(spi->bb, spi->mosi, (data_write & (1 << bit_count)) ? 1 : 0);
			ftdi_bitbang_write(spi->bb);
		}
		/* clock active (high or low depending on polarity) */
		ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol ? 0 : 1);
		ftdi_bitbang_write(spi->bb);
		/* read if phase is 0 */
		if (spi->cpha) {
			data_read = (data_read << 1) | ftdi_bitbang_read_pin(spi->bb, spi->miso);
		}
		/* write if phase is 1 */
		if (spi->cpha) {
			ftdi_bitbang_set_pin(spi->bb, spi->mosi, (data_write & (1 << bit_count)) ? 1 : 0);
			ftdi_bitbang_write(spi->bb);
		}
		/* clock inactive */
		ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol);
		ftdi_bitbang_write(spi->bb);
		/* read if phase is 1 */
		if (spi->cpha) {
			data_read = (data_read << 1) | ftdi_bitbang_read_pin(spi->bb, spi->miso);
		}
	}

	/* disable slave */
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, 1);
	}
	ftdi_bitbang_write(spi->bb);

	return data_read;
}
