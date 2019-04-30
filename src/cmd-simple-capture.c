/*
 * ftdi-bitbang
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <libftdi1/ftdi.h>
#include <signal.h>
#include "cmd-common.h"
#include "linkedlist.h"

const char opts[] = COMMON_SHORT_OPTS "p:t:s:l:";
struct option longopts[] = {
	COMMON_LONG_OPTS
	{ "pins", required_argument, NULL, 'p' },
	{ "trigger", required_argument, NULL, 't' },
	{ "speed", required_argument, NULL, 's' },
	{ "time", required_argument, NULL, 'l' },
	{ 0, 0, 0, 0 },
};

/* which pins state to read */
uint8_t pins_mask = 0xff;

/* trigger type: -1 is no trigger, 0 is falling edge, 1 is rising edge */
int trigger_type = -1;

/* trigger mask */
uint8_t trigger_mask = 0;

/* sampling speed */
int sampling_speed = 1e6;

/* sampling time */
double sampling_time = 1.0;

/* ftdi device context */
struct ftdi_context *ftdi = NULL;

unsigned int ftdi_chunksize;

struct sample {
	uint8_t *data;
	struct sample *prev;
	struct sample *next;
};
struct sample *sample_first = NULL;
struct sample *sample_last = NULL;
pthread_t sample_thread;
pthread_mutex_t sample_lock;
volatile int sample_exec = 0;

/**
 * Free resources allocated by process, quit using libraries, terminate
 * connections and so on. This function will use exit() to quit the process.
 *
 * @param return_code Value to be returned to parent process.
 */
void p_exit(int return_code)
{
	struct sample *sample;

	if (sample_exec) {
		sample_exec = 0;
		pthread_join(sample_thread, NULL);
	}
	pthread_mutex_destroy(&sample_lock);
	if (ftdi) {
		ftdi_free(ftdi);
	}

	LL_GET(sample_first, sample_last, sample);
	while (sample) {
		free(sample->data);
		free(sample);
		LL_GET(sample_first, sample_last, sample);
	}

	/* terminate program instantly */
	exit(return_code);
}

void p_help()
{
	printf(
	    "  -p, --pins=PINS[0-7]       pins to capture, default is '0,1,2,3,4,5,6,7'\n"
	    "  -t, --trigger=PIN[0-7]:EDGE\n"
	    "                             trigger from pin on rising or falling edge (EDGE = r or f),\n"
	    "                             if trigger is not set, sampling will start immediately\n"
	    "  -s, --speed=INT            sampling speed, default 1000000 (1 MS/s)\n"
	    "  -l, --time=FLOAT           sample for this many seconds, default 1 second\n"
	    "\n"
	    "Simple capture command for FTDI FTx232 chips.\n"
	    "Uses bitbang mode so only ADBUS (pins 0-7) can be sampled.\n"
	    "\n");
}

int p_options(int c, char *optarg)
{
	int i;
	char *token;
	switch (c) {
	case 'p':
		/* clear default to no pins */
		pins_mask = 0x00;
		for (token = strsep(&optarg, ","); token; token = strsep(&optarg, ",")) {
			int x = atoi(token);
			if (x < 0 || x > 7) {
				fprintf(stderr, "invalid pin: %d\n", x);
				return -1;
			}
			pins_mask |= (1 << x);
		}
		return 1;
	case 't':
		token = strsep(&optarg, ":");
		if (!optarg) {
			fprintf(stderr, "invalid trigger format\n");
			return -1;
		}
		i = atoi(token);
		if (i < 0 || i > 7) {
			fprintf(stderr, "invalid trigger pid: %d\n", i);
			return -1;
		}
		if (*optarg != 'r' && *optarg != 'f') {
			fprintf(stderr, "invalid trigger edge type: %s\n", optarg);
			return -1;
		}
		trigger_type = *optarg == 'r' ? 1 : 0;
		trigger_mask = (1 << i);
		return 1;
	case 's':
		sampling_speed = (int)atof(optarg);
		if (sampling_speed <= 0) {
			fprintf(stderr, "invalid sampling speed: %d\n", sampling_speed);
			return -1;
		}
		if (sampling_speed > 1e6) {
			fprintf(stderr, "NOTE: sampling speeds over 1 MS/s have been tested as unreliable\n");
		}
		return 1;
	case 'l':
		sampling_time = atof(optarg);
		if (sampling_time <= 0) {
			fprintf(stderr, "invalid sampling time: %f\n", sampling_time);
			return -1;
		}
		return 1;
	}

	return 0;
}

static long double os_time()
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (long double)((long double)tp.tv_sec + (long double)tp.tv_nsec / 1e9);
}

static void os_sleep(long double t)
{
	struct timespec tp;
	long double integral;
	t += os_time();
	tp.tv_nsec = (long)(modfl(t, &integral) * 1e9);
	tp.tv_sec = (time_t)integral;
	while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) == EINTR);
}

static void *sample_do(void *p)
{
	struct sample *sample = NULL;
	int n, c;

	/* capture data continuosly until told to stop */
	sample_exec = 1;
	while (sample_exec) {
		/* allocate new sample buffer */
		if (!sample) {
			sample = (struct sample *)malloc(sizeof(*sample));
			sample->data = (uint8_t *)malloc(ftdi_chunksize);
			n = 0;
		}
		/* read data into buffer */
		c = ftdi_read_data(ftdi, sample->data + n, ftdi_chunksize - n);
		if (c > 0) {
			n += c;
		} else if (c < 0) {
			fprintf(stderr, "sample read failure: %s\n", ftdi_get_error_string(ftdi));
			p_exit(EXIT_FAILURE);
		}
		/* continue read if buffer not full */
		if (n < ftdi_chunksize) {
			continue;
		}
		/* append sample into queue */
		pthread_mutex_lock(&sample_lock);
		LL_APP(sample_first, sample_last, sample);
		pthread_mutex_unlock(&sample_lock);
		sample = NULL;
	}

	return NULL;
}

static char *get_sampled_pins_str(uint8_t value)
{
	static char str[128];
	if (value & pins_mask) {
		strcpy(str, "1");
	} else {
		strcpy(str, "0");
	}
	return str;
}

void sig_catch_int(int signum)
{
	signal(signum, sig_catch_int);
	p_exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int err = 0, i;
	double t;
	struct sample *sample = NULL;

	signal(SIGINT, sig_catch_int);

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
	ftdi_set_latency_timer(ftdi, 1);
	// ftdi_write_data_set_chunksize(ftdi, FTDI_READ_BUFFER_SIZE);
	// ftdi_read_data_set_chunksize(ftdi, FTDI_READ_BUFFER_SIZE);
	ftdi_read_data_get_chunksize(ftdi, &ftdi_chunksize);
	ftdi_set_bitmode(ftdi, 0, BITMODE_RESET);
	if (ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG)) {
		fprintf(stderr, "unable to enable bitbang: %s\n", ftdi_get_error_string(ftdi));
		p_exit(EXIT_FAILURE);
	}
	if (ftdi_set_baudrate(ftdi, sampling_speed / 20)) {
		fprintf(stderr, "%s\n", ftdi_get_error_string(ftdi));
		p_exit(EXIT_FAILURE);
	}

	/* init sampling thread */
	pthread_mutex_init(&sample_lock, NULL);
	pthread_create(&sample_thread, NULL, sample_do, NULL);
	while (!sample_exec) {
		os_sleep(0.01);
	}

	/* set i to zero here, it wont be zero later if trigger was applied */
	i = 0;

	/* if trigger is set, wait for it */
	if (trigger_type > -1) {
		fprintf(stderr, "waiting for trigger on the %s edge with mask 0x%02x\n",
		        trigger_type == 1 ? "rising" : "falling",
		        trigger_mask);
	}
	while (trigger_type > -1) {
		/* if sample exists and no trigger occured */
		if (sample) {
			free(sample->data);
			free(sample);
		}
		/* get sample from queue */
		pthread_mutex_lock(&sample_lock);
		LL_GET(sample_first, sample_last, sample);
		pthread_mutex_unlock(&sample_lock);
		/* if no sample, sleep a little and try again */
		if (!sample) {
			os_sleep(0.01);
			continue;
		}
		/* check for trigger */
		for (i = 0; i < ftdi_chunksize; i++) {
			static int last_value = -1;
			/* fill last value if this is first sample */
			if (last_value < 0) {
				last_value = sample->data[i];
				continue;
			}
			/* check for trigger by type */
			if (trigger_type == 0 &&
			        (last_value & trigger_mask) &&
			        !(sample->data[i] & trigger_mask)) {
				fprintf(stderr, "falling edge trigger detected\n");
				trigger_type = -1;
				break;
			} else if (trigger_type == 1 &&
			           !(last_value & trigger_mask) &&
			           (sample->data[i] & trigger_mask)) {
				fprintf(stderr, "rising edge trigger detected\n");
				trigger_type = -1;
				break;
			}
			last_value = sample->data[i];
		}
	}

	/* read samples data */
	fprintf(stderr, "reading data...\n");
	for (t = 0.0; t <= sampling_time; ) {
		/* get next sample from queue */
		if (!sample) {
			pthread_mutex_lock(&sample_lock);
			LL_GET(sample_first, sample_last, sample);
			pthread_mutex_unlock(&sample_lock);
		}
		/* wait for more samples */
		if (!sample) {
			os_sleep(0.01);
			continue;
		}
		/* manage sample data */
		for ( ; i < ftdi_chunksize; i++) {
			static int last_value = -1;
			if (last_value < 0 ||
			        (last_value & pins_mask) != (sample->data[i] & pins_mask)) {
				fprintf(stdout, "%g,%s\n", t, get_sampled_pins_str(sample->data[i]));
			}
			last_value = sample->data[i];
			t += 1.0 / (double)sampling_speed;
			if (t > sampling_time) {
				break;
			}
		}
		i = 0;
		free(sample->data);
		free(sample);
		sample = NULL;
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

