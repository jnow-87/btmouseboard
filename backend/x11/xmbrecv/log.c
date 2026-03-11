#include <stdarg.h>
#include <stdio.h>
#include <backend/x11/log.h>
#include <backend/x11/opts.h>


/* global functions */
void log_msg(log_level_t level, char const *fmt, ...){
	FILE *fp = (level == LOG_ERROR) ? stderr : stdout;
	va_list lst;


	if(!opts.foreground || (level == LOG_DEBUG && !opts.debug))
		return;

	va_start(lst, fmt);
	vfprintf(fp, fmt, lst);
	fprintf(fp, "\n");
	va_end(lst);
}
