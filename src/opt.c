/*
 * Option parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opt.h"


/* instant exit when parsing of an option fails */
#define OPT_PARSING_FAILED_ACTION() do { opt_quit(); exit(EXIT_FAILURE); } while (0)
/* return error when parsing of an option fails */
// #define OPT_PARSING_FAILED_ACTION() do { return -1; } while (0)

#define OPT_IF_GET(CODE) \
	for (int i = 0; opts[i].name; i++) { \
		if (opts_in_use && !strchr(opts_in_use, opts[i].short_name)) { continue; } \
		if (opts[i].short_name == short_name) { CODE; } \
	}

/* internal functions */
static int opt_parse_single(struct opt_option *opt);

/* all options */
static struct opt_option *opts = NULL;
/* list of short option names to define actually used options or NULL for all */
static char *opts_in_use = NULL;
/* prepend help with this */
static const char *opt_help_prepend = NULL;
/* append this to help */
static const char *opt_help_append = NULL;


int opt_init(struct opt_option *o, const char *u, const char *help_prepend, const char *help_append)
{
	opts = o;
	opts_in_use = u ? strdup(u) : NULL;
	opt_help_prepend = help_prepend;
	opt_help_append = help_append;

	/* for checking overlapping options */
#ifdef DEBUG
	for (int i = 0; opts[i].name; i++) {
		for (int j = 0; opts[j].name; j++) {
			if (opts[i].short_name == opts[j].short_name && i != j) {
				printf("overlapping short option: %c\n", opts[i].short_name);
			}
		}
	}
#endif

	return 0;
}

void opt_quit(void)
{
	if (opts_in_use) {
		free(opts_in_use);
	}
	opts_in_use = NULL;

	for (int i = 0; opts[i].name; i++) {
		if (opts[i].used != 0 && opts[i].value) {
			free(opts[i].value);
		}
		opts[i].value = NULL;
	}
}

int opt_set_callback(int short_name, int (*callback)(int short_name, char *value))
{
	for (int i = 0; opts[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opts[i].short_name)) {
			continue;
		}
		if (opts[i].short_name == short_name) {
			opts[i].callback = callback;
			return 0;
		}
	}
	return -1;
}

int opt_parse(int argc, char *argv[])
{
	int err = 0;

	/* generate list of available options */
	size_t n = 0;
	if (opts_in_use) {
		n = strlen(opts_in_use);
	} else {
		for (n = 0; opts[n].name; n++);
	}
	char *shortopts = malloc(n * 2 + 1);
	struct option *longopts = malloc((n + 1) * sizeof(struct option));

	memset(shortopts, 0, n * 2 + 1);
	memset(longopts, 0, (n + 1) * sizeof(struct option));

	for (int i = 0, j = 0; opts[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opts[i].short_name)) {
			continue;
		}

		/* add to short opts */
		shortopts[strlen(shortopts)] = opts[i].short_name;
		if (opts[i].has_arg == required_argument) {
			strcat(shortopts, ":");
		}

		/* add to long opts */
		longopts[j].name = opts[i].name;
		longopts[j].has_arg = opts[i].has_arg;
		longopts[j].flag = NULL;
		longopts[j].val = opts[i].short_name;
		j++;
	}

	/* parse */
	int index = 0, c;
	while ((c = getopt_long(argc, argv, shortopts, longopts, &index)) > -1) {
		/* invalid option */
		if (c == '?') {
			exit(EXIT_FAILURE);
		}

		/* unfortunately this loop is required every time */
		for (int i = 0; opts[i].name; i++) {
			if (opts[i].short_name == c && opt_parse_single(&opts[i])) {
				err = -1;
				goto out;
			}
		}
	}

out:
	free(shortopts);
	free(longopts);
	return err;
}

void opt_help(void)
{
	if (opt_help_prepend) {
		printf("%s\n\n", opt_help_prepend);
	}

	printf("Options:\n");
	for (int i = 0; opts[i].name; i++) {
		if (opts_in_use && !strchr(opts_in_use, opts[i].short_name)) {
			continue;
		}

		int n = printf("  -%c, --%s%s", opts[i].short_name, opts[i].name, opts[i].has_arg != required_argument ? "" : "=VALUE");
		n < 30 ? printf("%*s", 30 - n, "") : printf("\n");

		/* print description */
		char *orig = strdup(opts[i].description);
		char *p = orig;
		for (int pad = n < 30 ? 0 : 30; p; pad = 30) {
			printf("%*s%s\n", pad, "", strsep(&p, "\n"));
		}
		free(orig);

		/* print default if set */
		if (opts[i].used <= 0 && opts[i].value) {
			printf("%*s(default: %s)\n", 30, "", opts[i].value);
		}
	}

	if (opt_help_append) {
		printf("\n%s\n", opt_help_append);
	}
}

int opt_used(int short_name)
{
	OPT_IF_GET({
		return opts[i].used > 0;
	});
	return 0;
}

int opt_set(int short_name, char *value)
{
	OPT_IF_GET({
		opts[i].value = strdup(value);
		opts[i].used = -1;
		return 0;
	});
	return -1;
}

char *opt_get(int short_name)
{
	OPT_IF_GET({
		return opts[i].value;
	});
	return NULL;
}

int opt_get_int(int short_name)
{
	OPT_IF_GET({
		if (opts[i].used < 1 && !opts[i].value)
		{
			return 0;
		}
		return (int)strtol(opts[i].value, NULL, 0);
	});
	return 0;
}


/* internals */


static int opt_parse_single(struct opt_option *opt)
{
	/* if help was asked, show and exit immediately */
	if (opt->short_name == 'h') {
		opt_help();
		exit(EXIT_SUCCESS);
	}

	/* if filter has a list of accepted values */
	if (opt->filter.type != OPT_FILTER_NONE && opt->filter.accept) {
		int found = 0;
		for (int i = 0; opt->filter.accept[i]; i++) {
			if (strcmp(opt->filter.accept[i], optarg) == 0) {
				found = 1;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "invalid value for option -%c, --%s: %s\n", opt->short_name, opt->name, optarg);
			OPT_PARSING_FAILED_ACTION();
		}
	}

	/* if filter type is number */
	if (opt->filter.type == OPT_FILTER_NUM) {
		char *p = NULL;
		double v = strtod(optarg, &p);
		if (strlen(p) > 0) {
			fprintf(stderr, "invalid numeric value for option -%c, --%s: %s\n", opt->short_name, opt->name, optarg);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v < opt->filter.min) {
			fprintf(stderr, "too small value for option -%c, --%s: %s (min: %lf)\n", opt->short_name, opt->name, optarg, opt->filter.min);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v > opt->filter.max) {
			fprintf(stderr, "too large value for option -%c, --%s: %s (max: %lf)\n", opt->short_name, opt->name, optarg, opt->filter.max);
			OPT_PARSING_FAILED_ACTION();
		}
	}

	/* if filter type is int */
	if (opt->filter.type == OPT_FILTER_INT) {
		char *p = NULL;
		long int v = strtol(optarg, &p, 10);
		if (strlen(p) > 0) {
			fprintf(stderr, "invalid integer value for option -%c, --%s: %s\n", opt->short_name, opt->name, optarg);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v < opt->filter.min) {
			fprintf(stderr, "too small value for option -%c, --%s: %s (min: %ld)\n", opt->short_name, opt->name, optarg, (long int)opt->filter.min);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v > opt->filter.max) {
			fprintf(stderr, "too large value for option -%c, --%s: %s (max: %ld)\n", opt->short_name, opt->name, optarg, (long int)opt->filter.max);
			OPT_PARSING_FAILED_ACTION();
		}
	}

	/* if filter type is hex */
	if (opt->filter.type == OPT_FILTER_HEX) {
		char *p = NULL;
		long int v = strtol(optarg, &p, 16);
		if (strlen(p) > 0) {
			fprintf(stderr, "invalid hex value for option -%c, --%s: %s\n", opt->short_name, opt->name, optarg);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v < opt->filter.min) {
			fprintf(stderr, "too small value for option -%c, --%s: %s (min: 0x%lx)\n", opt->short_name, opt->name, optarg, (long int)opt->filter.min);
			OPT_PARSING_FAILED_ACTION();
		}
		if (v > opt->filter.max) {
			fprintf(stderr, "too large value for option -%c, --%s: %s (max: 0x%lx)\n", opt->short_name, opt->name, optarg, (long int)opt->filter.max);
			OPT_PARSING_FAILED_ACTION();
		}
	}

	/* if callback is set */
	if (opt->callback && opt->callback(opt->short_name, optarg)) {
		OPT_PARSING_FAILED_ACTION();
	}

	/* save value */
	if (opt->used != 0 && opt->value) {
		free(opt->value);
	}
	opt->value = optarg ? strdup(optarg) : NULL;
	opt->used = 1;

	return 0;
}

