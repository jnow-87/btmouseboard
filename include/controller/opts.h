#ifndef OPTS_H
#define OPTS_H


#include <stdbool.h>


/* types */
typedef struct{
	bool debug,
		 log_to_stdout;
} opts_t;


/* global variables */
extern opts_t opts;


/* prototypes */
int opts_parse(int argc, char **argv);


#endif // OPTS_H
