/*
 * ftdi-bitbang
 *
 * Common routines for all command line utilies.
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef __CMD_COMMON_H__
#define __CMD_COMMON_H__

#include <getopt.h>

#define COMMON_SHORT_OPTS "hV:P:D:S:I:U:RL"
#define COMMON_LONG_OPTS \
    { "help", no_argument, NULL, 'h' }, \
    { "vid", required_argument, NULL, 'V' }, \
    { "pid", required_argument, NULL, 'P' }, \
    { "description", required_argument, NULL, 'D' }, \
    { "serial", required_argument, NULL, 'S' }, \
    { "interface", required_argument, NULL, 'I' }, \
    { "usbid", required_argument, NULL, 'U' }, \
    { "reset", no_argument, NULL, 'R' }, \
    { "list", no_argument, NULL, 'L' },


/**
 * Command specific option parsing.
 *
 * @param  c        option character
 * @param  optarg   optional argument
 * @return          1 if match, 0 if not
 */
int p_options(int c, char *optarg);

/**
 * Command specific exit.
 */
void p_exit(int return_code);

/**
 * Command specific help.
 */
void p_help();

/**
 * Print common help.
 */
void common_help(int argc, char *argv[]);

/**
 * Parse common options.
 */
int common_options(int argc, char *argv[], const char opts[], struct option longopts[]);

/**
 * Print list of matching devices.
 */
void common_ftdi_list_print();

/**
 * Initialize ftdi resources.
 *
 * @param argc Argument count.
 * @param argv Argument array.
 * @return 0 on success, -1 on errors.
 */
struct ftdi_context *common_ftdi_init();


#endif /* __CMD_COMMON_H__ */
