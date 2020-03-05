/*
 * Common FTDI handling.
 */

#ifndef FTDIC_H
#define FTDIC_H

#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>

int ftdic_init(void);
void ftdic_quit(void);
void ftdic_list(void);
struct ftdi_context *ftdic_get_context(void);

#endif /* FTDIC_H */
