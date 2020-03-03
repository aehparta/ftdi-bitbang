/*
 * Option parsing.
 */

#ifndef OPT_H
#define OPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <getopt.h>
#include <math.h>

enum {
	OPT_FILTER_NONE,
	OPT_FILTER_STR,
	OPT_FILTER_INT,
	OPT_FILTER_NUM,
	OPT_FILTER_HEX,
};

struct opt_filter {
	int type;
	double min;
	double max;
	const char **accept;
};

struct opt_option {
	int short_name;
	const char *name;
	int has_arg;
	int used; /* zero or less when not used (-1 when default set and value should be freed) */
	char *value;
	int (*callback)(int short_name, char *value);
	const char *description;
	struct opt_filter filter;
};

int opt_init(struct opt_option *opts, const char *use, const char *help_prepend, const char *help_append);
void opt_quit(void);

int opt_set_callback(int short_name, int (*callback)(int short_name, char *value));

int opt_parse(int argc, char *argv[]);

void opt_help(void);

int opt_used(int short_name);
int opt_set(int short_name, char *value);
char *opt_get(int short_name);
int opt_get_int(int short_name);

#ifdef __cplusplus
}
#endif

#endif /* OPT_H */
