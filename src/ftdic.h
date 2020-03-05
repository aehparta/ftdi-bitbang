/*
 * Common FTDI handling.
 */

#ifndef FTDIC_H
#define FTDIC_H

#include <stdint.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>

int ftdic_init(void);
int ftdic_init_broken(void);
void ftdic_quit(void);
void ftdic_list(void);
struct ftdi_context *ftdic_get_context(void);

int ftdic_bb_read_low(void);
int ftdic_bb_read_high(void);
int ftdic_bb_read(void);
int ftdic_bb_read_pin(uint8_t pin);

#endif /* FTDIC_H */
