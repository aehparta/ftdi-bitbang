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

struct ftdi_spi_context *ftdi_spi_init(struct ftdi_bitbang_context *bb, int sclk, int mosi, int miso, int ss)
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
	/* default clock polarity and phase */
	spi->cpol = 0;
	spi->cpha = 0;
	/* default select pin polarity */
	spi->sspol = 0;

	/* setup io pin directions */
	ftdi_bitbang_set_io(spi->bb, spi->sclk, 1);
	ftdi_bitbang_set_io(spi->bb, spi->mosi, 1);
	ftdi_bitbang_set_io(spi->bb, spi->miso, 0);
	ftdi_bitbang_set_io(spi->bb, spi->ss, 1);
	/* setup initial state */
	ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol);
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, spi->sspol ? 0 : 1);
	}
	ftdi_bitbang_write(spi->bb);

	return spi;
}

void ftdi_spi_free(struct ftdi_spi_context *spi)
{
	free(spi);
}

void ftdi_spi_set_mode(struct ftdi_spi_context *spi, int cpol, int cpha)
{
	spi->cpol = cpol ? 1 : 0;
	spi->cpha = cpha ? 1 : 0;
	/* change initial sclk state */
	ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol);
	ftdi_bitbang_write(spi->bb);
}

int ftdi_spi_enable(struct ftdi_spi_context *spi)
{
	/* enable slave */
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, spi->sspol);
		return ftdi_bitbang_write(spi->bb);
	}
	return 0;
}

int ftdi_spi_disable(struct ftdi_spi_context *spi)
{
	/* disable slave */
	if (spi->ss > -1) {
		ftdi_bitbang_set_pin(spi->bb, spi->ss, spi->sspol ? 0 : 1);
		return ftdi_bitbang_write(spi->bb);
	}
	return 0;
}

int ftdi_spi_transfer_do(struct ftdi_spi_context *spi, int data_write, int bit_count)
{
	int i;
	int data_read = 0;

	/* bit count to zero indexed */
	bit_count--;

	/* write first bit out now if phase is 0 (default) */
	if (!spi->cpha) {
		ftdi_bitbang_set_pin(spi->bb, spi->mosi, (data_write & (1 << bit_count)) ? 1 : 0);
		if (ftdi_bitbang_write(spi->bb) < 0) {
			return -1;
		}
	}

	/* transfer data */
	for (i = bit_count; i >= 0; i--) {
		/* write if phase is 1 */
		if (spi->cpha) {
			ftdi_bitbang_set_pin(spi->bb, spi->mosi, (data_write & (1 << bit_count)) ? 1 : 0);
		}
		/* clock active (high or low depending on polarity) */
		ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol ? 0 : 1);
		if (ftdi_bitbang_write(spi->bb) < 0) {
			return -1;
		}
		/* read and write if phase is 0 */
		if (!spi->cpha) {
			int v = ftdi_bitbang_read_pin(spi->bb, spi->miso);
			if (v < 0) {
				return -1;
			}
			data_read = (data_read << 1) | v;
			/* next bit */
			if (bit_count > 0) {
				ftdi_bitbang_set_pin(spi->bb, spi->mosi, (data_write & (1 << (bit_count - 1))) ? 1 : 0);
			}
		}
		/* clock inactive */
		ftdi_bitbang_set_pin(spi->bb, spi->sclk, spi->cpol);
		if (ftdi_bitbang_write(spi->bb) < 0) {
			return -1;
		}
		/* read if phase is 1 */
		if (spi->cpha) {
			int v = ftdi_bitbang_read_pin(spi->bb, spi->miso);
			if (v < 0) {
				return -1;
			}
			data_read = (data_read << 1) | v;
		}
	}

	return data_read;
}

int ftdi_spi_transfer(struct ftdi_spi_context *spi, int data_write, int bit_count)
{
	int data_read = 0;
	if (ftdi_spi_enable(spi) < 0) {
		return -1;
	}
	data_read = ftdi_spi_transfer_do(spi, data_write, bit_count);
	if (ftdi_spi_disable(spi) < 0) {
		return -1;
	}
	return data_read;
}
