/*
 * Argument parsing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "arg.h"


/* internal functions */
static int arg_read_stdin(struct arg *arg);


struct arg *arg_init(int argc, char *argv[], const char *sepsa, const char *sepsv, const char *stdina, const char *stdincc)
{
	struct arg *arg = malloc(sizeof(*arg));
	memset(arg, 0, sizeof(*arg));
	arg->v = argv;
	arg->c = argc;
	arg->sepsa = sepsa ? strdup(sepsa) : NULL;
	arg->sepsv = sepsv ? strdup(sepsv) : NULL;
	arg->stdina = stdina ? strdup(stdina) : NULL;
	arg->stdincc = stdincc ? strdup(stdincc) : NULL;
	return arg;
}

void arg_free(struct arg *arg)
{
	if (arg) {
		arg->sepsa ? free(arg->sepsa) : NULL;
		arg->sepsv ? free(arg->sepsv) : NULL;
		arg->stdina ? free(arg->stdina) : NULL;
		arg->stdincc ? free(arg->stdincc) : NULL;
		arg->a ? free(arg->a) : NULL;
		free(arg);
	}
}

int arg_parse(struct arg *arg)
{
	/* check if no more data */
	if (!arg->a && arg->c <= arg->i) {
		return 0;
	}

	/* if no more data in current argument */
	if (arg->a && !arg->next) {
		free(arg->a);
		arg->a = NULL;
		if (arg->using_stdin != 1) {
			arg->i++;
		}
		arg->has_sepsa = 0;
		return arg_parse(arg);
	}

	/* if new argument */
	if (!arg->a) {
		arg->next = arg->a = strdup(arg->v[arg->i]);
	}

	/* if using stdin */
	if ((arg->using_stdin == 1 && !arg->a) || (arg->stdina && strcmp(arg->stdina, arg->a) == 0)) {
		/* check that stdin is fifo or regular file */
		if (!arg->using_stdin) {
			struct stat st;
			if (fstat(fileno(stdin), &st) || (!S_ISFIFO(st.st_mode) && !S_ISREG(st.st_mode))) {
				fprintf(stderr, "stdin is not a pipe or redirected file, skip reading stdin\n");
				arg->next = NULL;
				return arg_parse(arg);
			}
			arg->using_stdin = 1;
		}

		/* get data from stdin but first check if stdin has already been read */
		if (arg->using_stdin == 2 || arg_read_stdin(arg)) {
			arg->using_stdin = 2;
			arg->next = NULL;
			return arg_parse(arg);
		}
	}

	/* if arguments inside argument separator is defined */
	if (arg->sepsa) {
		arg->last_name = strsep(&arg->next, arg->sepsa);
		arg->has_sepsa = arg->next != NULL;
	} else {
		arg->last_name = arg->a;
		arg->next = NULL;
	}

	/* if name and value separator defined */
	if (arg->sepsv) {
		arg->last_value = arg->last_name;
		strsep(&arg->last_value, arg->sepsv);
	} else {
		arg->last_value = NULL;
	}

	return arg->last_value ? 2 : 1;
}

const char *arg_name(struct arg *arg)
{
	return arg->last_name;
}

const char *arg_value(struct arg *arg)
{
	return arg->last_value;
}

int arg_has_sepsa(struct arg *arg)
{
	return arg->has_sepsa;
}


/* internals */


static int arg_read_stdin(struct arg *arg)
{
	int c;

	/* remove spaces */
	for (c = fgetc(stdin); isspace(c); c = fgetc(stdin));
	if (c == EOF) {
		return EOF;
	}

	/* if comment character, keep reading until line feed */
	if (arg->stdincc && strchr(arg->stdincc, c)) {
		for (c = fgetc(stdin); c != '\n' && c != EOF; c = fgetc(stdin));
		return arg_read_stdin(arg);
	}

	/* clear current buffer */
	arg->a ? free(arg->a) : NULL;
	arg->a = NULL;

	/* read until next space */
	ungetc(c, stdin);
	if (scanf("%ms", &arg->a) < 1) {
		return -1;
	}
	arg->next = arg->a;

	return 0;
}
