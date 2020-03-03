
#include <stdio.h>
#include "opt-all.h"

int cb(int c, char *v)
{
	printf("cb %c: %s\n", c, v);
	return 0;
}

int main(int argc, char *argv[])
{
	// opt_init("hVPDR");
	opt_init(opt_all, NULL, NULL, NULL);

	opt_set_callback('R', cb);
	opt_set_callback('V', cb);

	opt_set('P', "123456");

	opt_parse(argc, argv);

	printf("R was %s\n", opt_used('R') ? "used" : "not used");
	printf("P: %s\n", opt_get('P'));
	
	opt_quit();
	return 0;
}
