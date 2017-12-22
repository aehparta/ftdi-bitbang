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
	int sspol;
};

struct ftdi_spi_context *ftdi_spi_init(struct ftdi_bitbang_context *bb, int sclk, int mosi, int miso, int ss);
void ftdi_spi_free(struct ftdi_spi_context *spi);
void ftdi_spi_set_mode(struct ftdi_spi_context *spi, int cpol, int cpha);

/**
 * Enable SPI slave.
 * @param  spi        spi context
 * @return            0 on success or -1 on errors
 */
int ftdi_spi_enable(struct ftdi_spi_context *spi);

/**
 * Disable SPI slave.
 * @param  spi        spi context
 * @return            0 on success or -1 on errors
 */
int ftdi_spi_disable(struct ftdi_spi_context *spi);

/**
 * Transfer data to and from SPI slave.
 * You need to enable slave first. Use ftdi_spi_enable() or
 * set ss pin active manually.
 * 
 * @param  spi        spi context
 * @param  data_write data bits to be written
 * @param  bit_count  bit count to transfer, read and write
 * @return            data bits read or -1 on errors
 */
int ftdi_spi_transfer_do(struct ftdi_spi_context *spi, int data_write, int bit_count);

/**
 * Does same as ftdi_spi_transfer_do() but with automatic slave enable/disable.
 * 
 * @param  spi        spi context
 * @param  data_write data bits to be written
 * @param  bit_count  bit count to transfer, read and write
 * @return            data bits read or -1 on errors
 */
int ftdi_spi_transfer(struct ftdi_spi_context *spi, int data_write, int bit_count);


#endif /* __FTDI_SPI_H__ */
