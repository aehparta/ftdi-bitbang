/*
 * Common FTDI handling.
 */

#ifndef FTDIC_H
#define FTDIC_H

#include <stdint.h>
#include <stdbool.h>
#include <ftdi.h>
#include <libusb.h>


int ftdic_init(void);
int ftdic_init_broken(void);
void ftdic_quit(void);
void ftdic_list(void);
struct ftdi_context *ftdic_get_context(void);

int ftdic_bb_read_low(void);
int ftdic_bb_read_high(void);
int ftdic_bb_read(void);
int ftdic_bb_read_pin(uint8_t pin);

int ftdic_bb_dir_io(uint16_t dir);
int ftdic_bb_dir_io_pin(uint8_t pin, bool out);
int ftdic_bb_set_pins(uint16_t pins);
int ftdic_bb_set_pin(uint8_t pin, bool high);
int ftdic_bb_flush(void);


#endif /* FTDIC_H */
