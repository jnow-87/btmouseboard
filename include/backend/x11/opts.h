#ifndef OPTS_H
#define OPTS_H


#include <stdbool.h>
#include <stdint.h>


/* types */
typedef struct{
	uint16_t port;
	bool foreground,
		 debug;
} opts_t;


/* global variables */
extern opts_t opts;


/* prototypes */
int opts_parse(int argc, char **argv);


#endif // OPTS_H
