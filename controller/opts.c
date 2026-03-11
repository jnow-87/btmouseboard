#include <config/config.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <controller/opts.h>


/* local/static prototypes */
static int help(char const *prog_name, char const *err, ...);
static backend_type_t strbackend(char const *s);


/* global variables */
opts_t opts = {
	.debug = false,
	.log_to_stdout = false,
	.reverse_custom_xkb_map = false,
	.port = CONFIG_SOCK_PORT,
	.backend = BE_BLUETOOTH,
	.host = 0x0,
};


/* global functions */
int opts_parse(int argc, char **argv){
	int opt;
	struct option const long_opt[] = {
		{ .name = "port",					.has_arg = required_argument,	.flag = 0x0,	.val = 'p' },
		{ .name = "backend",				.has_arg = required_argument,	.flag = 0x0,	.val = 'b' },
		{ .name = "debug",					.has_arg = no_argument,			.flag = 0x0,	.val = 'd' },
		{ .name = "log-to-stdout",			.has_arg = no_argument,			.flag = 0x0,	.val = 's' },
		{ .name = "reverse-xkb-custom-map",	.has_arg = no_argument,			.flag = 0x0,	.val = 'x' },
		{ .name = "help",					.has_arg = no_argument,			.flag = 0x0,	.val = 'h' },
		{ 0, 0, 0, 0}
	};


	while((opt = getopt_long(argc, argv, ":p:b:dsxh", long_opt, 0)) != -1){
		switch(opt){
		case 'p':	opts.port = atoi(optarg); break;
		case 'b':	opts.backend = strbackend(optarg); break;
		case 'd':	opts.debug = true; break;
		case 's':	opts.log_to_stdout = true; break;
		case 'x':	opts.reverse_custom_xkb_map = true; break;
		case 'h':	return help(argv[0], 0x0);

		case ':':	return help(argv[0], "missing argument to \"%c\"\n\n", optopt);
		case '?':	return help(argv[0], "invalid option \"%c\"\n\n", optopt);
		default:	return help(argv[0], "unknown error\n\n");
		}
	}

	if(opts.backend == BE_INVAL)
		return help(argv[0], "unknown backend\n");

	if(opts.backend == BE_X11){
		if(argc - optind <= 0)
			return help(argv[0], "missing host\n");

		opts.host = argv[optind++];
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
		"usage: %s (mouseboard) [options] <arguments>\n"
		"\n"
		"Grab the X11 keyboard and mouse and redirect their inputs through the selected backend.\n"
		"\n"
		"Arguments:\n"
		"    %-20.20s    %s\n"
		"\n"
		"Options:\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s (default=%u)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s (default=%s)\n"
		"    %-20.20s    %s\n"
		, prog_name
		// arguments
		, "<hostname | ip>", "target host when using the x11 backend"

		// options
		, "-b, --backend=<backend>", "backend to use, select either of bluetooth or x11", "bluetooth"
		, "-p, --port=<port>", "tcp port use for the x11 backend", CONFIG_SOCK_PORT
		, "-d, --debug", "enable debug output", "false"
		, "-s, --log-to-stdout", "print log message to stdout rather than the application window", "false"
		, "-x, --reverse-custom-xkb-map", "reverse the effects of the custom xkb map", "false"
		, "-h, --help", "print this help message"
	);

	return (err == 0x0) ? 1 : -1;
}

static backend_type_t strbackend(char const *s){
	if(strcmp(s, "bluetooth") == 0)	return BE_BLUETOOTH;
	if(strcmp(s, "x11") == 0)		return BE_X11;

	return BE_INVAL;
}
