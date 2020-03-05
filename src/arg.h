/*
 * Argument parsing.
 */

#ifndef ARG_H
#define ARG_H

#ifdef __cplusplus
extern "C" {
#endif

struct arg {
	char **v;
	int c;
	int i;

	char *sepsa;
	char *sepsv;
	
	char *stdina;
	char *stdincc;
	int using_stdin;

	char *a;
	char *last_name;
	char *last_value;
	char *next;
	int was_sepsa;
};

struct arg *arg_init(int argc, char *argv[], const char *sepsa, const char *sepsv, const char *stdina, const char *stdincc);
void arg_free(struct arg *arg);

int arg_parse(struct arg *arg);

const char *arg_name(struct arg *arg);
const char *arg_value(struct arg *arg);

int arg_was_sepsa(struct arg *arg);

#ifdef __cplusplus
}
#endif

#endif /* ARG_H */
