#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <controller/opts.h>


/* local/static prototypes */
static int help(char const *prog_name, char const *err, ...);


/* global variables */
opts_t opts = {
	.debug = false,
	.log_to_stdout = false,
};


/* global functions */
int opts_parse(int argc, char **argv){
	int opt;
	struct option const long_opt[] = {
		{ .name = "debug",			.has_arg = no_argument,	.flag = 0x0,	.val = 'd' },
		{ .name = "log-to-stdout",	.has_arg = no_argument,	.flag = 0x0,	.val = 's' },
		{ .name = "help",			.has_arg = no_argument,	.flag = 0x0,	.val = 'h' },
		{ 0, 0, 0, 0}
	};


	while((opt = getopt_long(argc, argv, ":dsh", long_opt, 0)) != -1){
		switch(opt){
		case 'd':	opts.debug = true; break;
		case 's':	opts.log_to_stdout = true; break;
		case 'h':	return help(argv[0], 0x0);

		case ':':	return help(argv[0], "missing argument to \"%s\"\n\n", argv[optind - 1]);
		case '?':	return help(argv[0], "invalid option \"%s\"\n\n", argv[optind - 1]);
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
		"Grab the X11 keyboard and mouse and redirect their inputs to a btmouseboard usb or uart device.\n"
		"\n"
		"Options:\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s\n"
		, prog_name
		, "-d, --debug", "enable debug output", "false"
		, "-s, --log-to-stdout", "print log message to stdout rather than the application window", "false"
		, "-h, --help", "print this help message"
	);

	return (err == 0x0) ? 1 : -1;
}


