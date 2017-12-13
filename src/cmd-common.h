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

/**
 * Initialize ftdi resources.
 *
 * @param argc Argument count.
 * @param argv Argument array.
 * @return 0 on success, -1 on errors.
 */
struct ftdi_context *cmd_init(int vid, int pid, const char *description, const char *serial, int interface);


#endif /* __CMD_COMMON_H__ */
