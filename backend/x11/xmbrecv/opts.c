#include <config/config.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <backend/x11/opts.h>


/* local/static prototypes */
static int help(char const *prog_name, char const *err, ...);


/* global variables */
opts_t opts = {
	.port = CONFIG_SOCK_PORT,
	.foreground = false,
	.debug = false,
};


/* global functions */
int opts_parse(int argc, char **argv){
	int opt;
	struct option const long_opt[] = {
		{ .name = "port",		.has_arg = required_argument,	.flag = 0x0,	.val = 'p' },
		{ .name = "foreground",	.has_arg = no_argument,			.flag = 0x0,	.val = 'f' },
		{ .name = "debug",		.has_arg = no_argument,			.flag = 0x0,	.val = 'd' },
		{ .name = "help",		.has_arg = no_argument,			.flag = 0x0,	.val = 'h' },
		{ 0, 0, 0, 0}
	};


	while((opt = getopt_long(argc, argv, ":p:fdh", long_opt, 0)) != -1){
		switch(opt){
		case 'p':	opts.port = atoi(optarg); break;
		case 'f':	opts.foreground = true; break;
		case 'd':	opts.debug = true; break;
		case 'h':	return help(argv[0], 0x0);

		case ':':	return help(argv[0], "missing argument to \"%c\"\n\n", optopt);
		case '?':	return help(argv[0], "invalid option \"%c\"\n\n", optopt);
		default:	return help(argv[0], "unknown error\n\n");
		}
	}

	if(argc - optind > 0)
		return help(argv[0], "too many arguments\n");

	return 0;
}


/* local functions */
static int help(char const *prog_name, char const *err, ...){
	va_list lst;


	if(err != 0x0 && *err != 0){
		va_start(lst, err);
		vprintf(err, lst);
		va_end(lst);
	}

	printf(
		"usage: %s [options]\n"
		"\n"
		"X11 daemon to create X server events based on commands from the xmouseboard (xmb).\n"
		"\n"
		"Options:\n"
		"    %-20.20s    %s (default=%u)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s\n"
		, prog_name
		, "-p, --port=<port>", "set port to await xmb commands", CONFIG_SOCK_PORT
		, "-f, --foreground", "do not run as daemon but in foreground", "false"
		, "-d, --debug", "enable debug output", "false"
		, "-h, --help", "print this help message"
	);

	return (err == 0x0) ? 1 : -1;
}
