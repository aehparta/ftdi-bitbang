/*
 * ftdi-spi
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include "ftdi-bitbang.h"
#include "ftdi-spi.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "c:o:i:s:la";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "sclk", required_argument, NULL, 'c' },
	{ "mosi", required_argument, NULL, 'o' },
	{ "miso", required_argument, NULL, 'i' },
	{ "ss", required_argument, NULL, 's' },
	{ "cpol", no_argument, NULL, 'l' },
	{ "cpha", no_argument, NULL, 'a' },
	{ 0, 0, 0, 0 },
};

int sclk = 0;
int mosi = 1;
int miso = 2;
int ss = 3;
int cpol = 0;
int cpha = 0;

/* ftdi device context */
struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_context *device = NULL;
struct ftdi_spi_context *spi = NULL;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
	if (spi) {
		ftdi_spi_free(spi);
	}
	if (device) {
		ftdi_bitbang_save_state(device);
		ftdi_bitbang_free(device);
	}
	if (ftdi) {
		ftdi_free(ftdi);
	}
	/* terminate program instantly */
	exit(return_code);
}

void p_help()
{
	printf(
	    "  -c, --sclk=PIN             SPI SCLK, default pin is 0\n"
	    "  -o, --mosi=PIN             SPI MOSI, default pin is 1\n"
	    "  -i, --miso=PIN             SPI MISO, default pin is 2\n"
	    "  -s, --ss=PIN               SPI SS, default pin is 3\n"
	    "  -l, --cpol1                set SPI CPOL to 1 (default 0)\n"
	    "  -a, --cpha1                set SPI CPHA to 1 (default 0)\n"
	    "\n"
	    "Bitbang style SPI through FTDI FTx232 chips.\n"
	    "\n");
}


int p_options(int c, char *optarg)
{
	switch (c) {
	case 'c':
		sclk = atoi(optarg);
		return 1;
	case 'o':
		mosi = atoi(optarg);
		return 1;
	case 'i':
		miso = atoi(optarg);
		return 1;
	case 's':
		ss = atoi(optarg);
		return 1;
	case 'l':
		cpol = 1;
		return 1;
	case 'a':
		cpha = 1;
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0, i;
	uint8_t buf[24];

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = common_ftdi_init();
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}
	ftdi_bitbang_load_state(device);



	ftdi_write_data_set_chunksize(ftdi, 4);
	ftdi_write_data_get_chunksize(ftdi, &i);
	printf("chunk size: %d\n", i);
	ftdi_set_latency_timer(ftdi, 1);
	ftdi_get_latency_timer(ftdi, &i);
	printf("latency: %d\n", i);

	ftdi_bitbang_set_io(device, 0, 1);
	while (1) {
		ftdi_bitbang_set_pin(device, 0, 0);
		ftdi_bitbang_write(device);
		ftdi_bitbang_set_pin(device, 0, 1);
		ftdi_bitbang_write(device);
	}
	
	/* initialize spi */
	spi = ftdi_spi_init(device, sclk, mosi, miso, ss);
	if (!spi) {
		fprintf(stderr, "ftdi_spi_init() failed\n");
		return -1;
	}
	ftdi_spi_set_mode(spi, cpol, cpha);

	/* run commands */
	ftdi_spi_enable(spi);
	ftdi_spi_transfer_do(spi, 0x060, 11);
	printf("0x%04x\n", ftdi_spi_transfer_do(spi, 0, 13));
	ftdi_spi_disable(spi);

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

