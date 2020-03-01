/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftdi1/ftdi.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include "ftdi-bitbang.h"
#include "cmd-common.h"

const char opts[] = COMMON_SHORT_OPTS "m:b:c:d:r:";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "mode", required_argument, NULL, 'm' },
	{ "baudrate", required_argument, NULL, 'b' },
	{ "pgc", required_argument, NULL, 'c' },
	{ "pgd", required_argument, NULL, 'd' },
	{ "mclr", required_argument, NULL, 'r' },
	{ 0, 0, 0, 0 },
};

/* ftdi device context */
struct ftdi_context *ftdi = NULL;
struct ftdi_bitbang_context *device = NULL;
int bitmode = 0;
int baudrate = 0;

int pgc = 0;
int pgd = 1;
int mclr = 3;

/* accurate timing routines */
static long double _os_time()
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (long double)((long double)tp.tv_sec + (long double)tp.tv_nsec / 1e9);
}
static void _os_sleep(long double t)
{
	struct timespec tp;
	long double integral;
	t += _os_time();
	tp.tv_nsec = (long)(modfl(t, &integral) * 1e9);
	tp.tv_sec = (time_t)integral;
	while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) == EINTR);
}

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
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
	    "  -b, --baudrate=INT         set baudrate to given, default is 1MHz\n"
	    "  -c, --pgc                  clock pin, default 0\n"
	    "  -d, --pgd                  data pin, default 1\n"
	    "  -r, --mclr                 reset pin, default 3\n"
	    "\n"
	    "PIC microcontroller LVP programmer.\n"
	    "\n");
}

int p_options(int c, char *optarg)
{
	int i;
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
	case 'b':
		baudrate = atoi(optarg);
		if (baudrate < 1 || baudrate > 20e6) {
			fprintf(stderr, "invalid baudrate, maximum is 20MHz\n");
			p_exit(1);
		}
		return 1;
	case 'c':
		pgc = atoi(optarg);
		return 1;
	case 'd':
		pgd = atoi(optarg);
		return 1;
	case 'r':
		mclr = atoi(optarg);
		return 1;
	}

	return 0;
}

/* empty clocks */
void icsp_nc(int n)
{
	ftdi_bitbang_set_pin(device, pgd, 0);
	for (; n > 0; n--) {
		ftdi_bitbang_set_pin(device, pgc, 1);
		ftdi_bitbang_write(device);
		ftdi_bitbang_set_pin(device, pgc, 0);
		ftdi_bitbang_write(device);
	}
}

void icsp_cmd(uint8_t cmd)
{
	/* send command */
	for (uint8_t mask = 1; mask < 0x40; mask = mask << 1) {
		ftdi_bitbang_set_pin(device, pgc, 1);
		ftdi_bitbang_set_pin(device, pgd, cmd & mask);
		ftdi_bitbang_write(device);
		ftdi_bitbang_set_pin(device, pgc, 0);
		ftdi_bitbang_write(device);
	}

	/* tdly delay */
	_os_sleep(0.001);

	/* clock start bit */
	icsp_nc(1);
}

void icsp_wr(uint8_t *data, size_t size)
{
	for (; size > 0; size--, data++) {
		for (uint8_t b = *data, mask = 1; mask; mask = mask << 1) {
			ftdi_bitbang_set_pin(device, pgc, 1);
			ftdi_bitbang_set_pin(device, pgd, b & mask);
			ftdi_bitbang_write(device);
			ftdi_bitbang_set_pin(device, pgc, 0);
			ftdi_bitbang_write(device);
		}
	}
}

void icsp_load_pc(uint16_t addr)
{
	icsp_cmd(0x1d);
	icsp_wr((uint8_t *)&addr, 2);
	icsp_nc(7);
}

void icsp_rd(uint16_t addr, uint8_t *data, size_t size)
{
	icsp_load_pc(addr);
	_os_sleep(0.01);

	for (; size > 0; size -= 2) {
		uint16_t w = 0;

		icsp_cmd(0x24);

		/* data as input */
		ftdi_bitbang_set_io(device, pgd, 0);
		ftdi_bitbang_write(device);

		for (uint16_t mask = 1; mask < 0x4000; mask = mask << 1) {
			ftdi_bitbang_set_pin(device, pgc, 1);
			ftdi_bitbang_write(device);
			ftdi_bitbang_set_pin(device, pgc, 0);
			ftdi_bitbang_write(device);
			uint16_t pins = ftdi_bitbang_read(device);
			if (pins & (1 << pgd)) {
				w |= mask;
			}
		}

		/* data back as output */
		ftdi_bitbang_set_io(device, pgd, 1);
		ftdi_bitbang_write(device);

		/* stop bit */
		icsp_nc(1);

		*data = w >> 8;
		data++;
		*data = w & 0xff;
		data++;
	}
}

int main(int argc, char *argv[])
{
	int err = 0;

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts, 0, 1)) {
		fprintf(stderr, "invalid command line option(s)\n");
		p_exit(EXIT_FAILURE);
	}

	/* init ftdi things */
	ftdi = common_ftdi_init();
	if (!ftdi) {
		p_exit(EXIT_FAILURE);
	}

	/* initialize to bitbang mode */
	device = ftdi_bitbang_init(ftdi, bitmode, 1, baudrate);
	if (!device) {
		fprintf(stderr, "ftdi_bitbang_init() failed\n");
		p_exit(EXIT_FAILURE);
	}

	/* initialize programmer pins */
	err = ftdi_bitbang_set_io(device, pgc, 1);
	err += ftdi_bitbang_set_pin(device, pgc, 0);
	if (err != 0) {
		fprintf(stderr, "invalid pin for clock\n");
		p_exit(EXIT_FAILURE);
	}
	err = ftdi_bitbang_set_io(device, pgd, 1);
	err += ftdi_bitbang_set_pin(device, pgd, 0);
	if (err != 0) {
		fprintf(stderr, "invalid pin for data\n");
		p_exit(EXIT_FAILURE);
	}
	err = ftdi_bitbang_set_io(device, mclr, 1);
	err += ftdi_bitbang_set_pin(device, mclr, 0);
	if (err != 0) {
		fprintf(stderr, "invalid pin for reset\n");
		p_exit(EXIT_FAILURE);
	}
	if (ftdi_bitbang_write(device) < 0) {
		fprintf(stderr, "failed setting pin states for programming\n");
		p_exit(EXIT_FAILURE);
	}

	/* enter LVP mode */
	_os_sleep(0.01);
	icsp_wr("PHCM MCHP", 4);
	_os_sleep(0.01);

	uint8_t x[4];
	icsp_rd(0x0000, x, 4);
	printf("%02x %02x %02x %02x\n", x[0], x[1], x[2], x[3]);

	/* release pins */
	_os_sleep(0.01);
	ftdi_bitbang_set_io(device, pgc, 0);
	ftdi_bitbang_set_io(device, pgd, 0);
	ftdi_bitbang_set_io(device, mclr, 0);
	ftdi_bitbang_set_pin(device, mclr, 1);
	ftdi_bitbang_write(device);

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

