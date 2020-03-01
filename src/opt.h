/*
 * Option parsing.
 */

#ifndef OPT_H
#define OPT_H

int opt_init(char *use);
void opt_quit(void);

int opt_set_callback(int short_name, int (*callback)(int short_name, char *value));

int opt_parse(int argc, char *argv[]);

void opt_help(void);

int opt_used(int short_name);
int opt_set(int short_name, char *value);
char *opt_get(int short_name);

#endif /* OPT_H */
