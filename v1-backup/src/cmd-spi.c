/*
 * ftdi-spi
 *
 * Only bitbang mode supported for now, built-in MPSSE maybe in the future.
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

const char opts[] = COMMON_SHORT_OPTS "m:c:o:i:s:ladn:CX";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "mode", required_argument, NULL, 'm' },
	{ "sclk", required_argument, NULL, 'c' },
	{ "mosi", required_argument, NULL, 'o' },
	{ "miso", required_argument, NULL, 'i' },
	{ "ss", required_argument, NULL, 's' },
	{ "cpol", no_argument, NULL, 'l' },
	{ "cpha", no_argument, NULL, 'a' },
	{ "dec", no_argument, NULL, 'd' },
	{ "size", no_argument, NULL, 'n' },
	{ "csv", no_argument, NULL, 'C' },
	{ "0x", no_argument, NULL, 'X' },
	{ 0, 0, 0, 0 },
};

int sclk = 0;
int mosi = 1;
int miso = 2;
int ss = 3;
int cpol = 0;
int cpha = 0;
int hex_or_dec = 0;
int size_of_data = 0;
int csv = 0;
int add_0x = 0;

/* ftdi device context */
struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_context *device = NULL;
struct ftdi_spi_context *spi = NULL;
int bitmode = 0;

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
	    "  -m, --mode=STRING          set device bitmode, use 'bitbang' or 'mpsse', default is 'bitbang'\n"
	    "                             for bitbang mode the baud rate is fixed to 1 MHz for now\n"
	    "  -c, --sclk=PIN             SPI SCLK, default pin is 0\n"
	    "  -o, --mosi=PIN             SPI MOSI, default pin is 1\n"
	    "  -i, --miso=PIN             SPI MISO, default pin is 2\n"
	    "  -s, --ss=PIN               SPI SS, default pin is 3\n"
	    "  -l, --cpol1                set SPI CPOL to 1 (default 0)\n"
	    "  -a, --cpha1                set SPI CPHA to 1 (default 0)\n"
	    "  -n, --size=INT             size of data to write, default is the count of bytes given as arguments\n"
	    "                             if less bytes is given as arguments than this value,\n"
	    "                             the last byte is repeated to fill the size\n"
	    "  -d, --dec                  values use decimal, both input and printed (default is hex)\n"
	    "  -X, --0x                   add 0x to start of each printed hex value (use only without -d)\n"
	    "  -C, --csv                  output as csv\n"
	    "\n"
	    "Bitbang style SPI through FTDI FTx232 chips.\n"
	    "\n"
	    "Examples:\n"
	    " transfer 4 bytes:    ftdi-spi ff 00 ff 5a\n"
	    " same using decimals: ftdi-spi -d 255 00 255 90\n"
	    "\n");
}


int p_options(int c, char *optarg)
{
	switch (c) {
	case 'm':
		if (strcmp("bitbang", optarg) == 0) {
			bitmode = BITMODE_BITBANG;
		} else if (strcmp("mpsse", optarg) == 0) {
			bitmode = BITMODE_MPSSE;
		} else {
			fprintf(stderr, "invalid bitmode\n");
			return -1;
		}
		return 1;
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
	case 'd':
		hex_or_dec = 1;
		return 1;
	case 'n':
		size_of_data = atoi(optarg);
		if (size_of_data < 1) {
			fprintf(stderr, "invalid data size\n");
			return -1;
		}
		return 1;
	case 'C':
		csv = 1;
		return 1;
	case 'X':
		add_0x = 1;
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int err = 0, i;
	uint8_t *out = NULL, *in = NULL;
	size_t size = 0;

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts, 1, 0)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* parse write data */
	for (i = optind; i < argc; i++) {
		int v = strtoul(argv[i], NULL, hex_or_dec ? 10 : 16);
		if (v < 0 || v > 255) {
			fprintf(stderr, "invalid value given: %s\n", argv[i]);
			p_exit(EXIT_FAILURE);
		}
		out = realloc(out, size + 1);
		out[size] = v;
		size++;
	}
	if (size > size_of_data && size_of_data > 0) {
		size = size_of_data;
	} else if (size < size_of_data) {
		out = realloc(out, size_of_data);
		memset(out + size, out[size - 1], size_of_data - size);
		size = size_of_data;
	}

	/* allocate input buffer (which is also used as transfer buffer, so copy output to it) */
	in = malloc(size);
	memcpy(in, out, size);

	/* print data */
	if (csv) {
		printf("send,recv\n");
		for (i = 0; i < size; i++) {
			if (hex_or_dec) {
				printf("%u,%u\n", out[i], in[i]);
			} else {
				printf("%s%02X,%s%02X\n", add_0x ? "0x" : "", out[i], add_0x ? "0x" : "", in[i]);
			}
		}
	} else {
		printf("send: ");
		for (i = 0; i < size; i++) {
			if (hex_or_dec) {
				printf("%3u%s", out[i], ((i + 1) < size) ? " " : "");
			} else {
				printf("%s%02X%s", add_0x ? "0x" : "", out[i], ((i + 1) < size) ? " " : "");
			}
		}
		printf("\n");
		printf("recv: ");
		for (i = 0; i < size; i++) {
			if (hex_or_dec) {
				printf("%3u%s", in[i], ((i + 1) < size) ? " " : "");
			} else {
				printf("%s%02X%s", add_0x ? "0x" : "", in[i], ((i + 1) < size) ? " " : "");
			}
		}
		printf("\n");
	}


	return 0;



	/* init ftdi things */
	ftdi = common_ftdi_init();
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi, bitmode, 1, 0);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}

	/* initialize spi */
	spi = ftdi_spi_init(device, sclk, mosi, miso, ss);
	if (!spi) {
		fprintf(stderr, "ftdi_spi_init() failed\n");
		return -1;
	}
	ftdi_spi_set_mode(spi, cpol, cpha);

	/* run commands */
	// ftdi_spi_enable(spi);
	// ftdi_spi_transfer_do(spi, 0x060, 11);
	// printf("0x%04x\n", ftdi_spi_transfer_do(spi, 0, 13));
	// ftdi_spi_disable(spi);

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

