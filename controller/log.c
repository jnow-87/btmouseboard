#include <config/config.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <controller/log.h>
#include <controller/opts.h>
#include <controller/render.h>


/* macros */
#define INC_IDX(idx, inc)	idx = (((idx) + (inc)) % CONFIG_LOG_MAX)


/* static variables */
static log_entry_t log[CONFIG_LOG_MAX] = { 0 };
static size_t log_start = 0,
			  log_end = 0,
			  log_next = 0;
static log_level_t log_level = 0;


/* global functions */
int log_init(bool debug){
	log_level = LOG_INFO | LOG_ERROR | (debug ? LOG_DEBUG : 0x0);
}

void log_add(log_level_t level, char const *fmt, ...){
	log_entry_t *entry = log + log_end;
	char text[LINE_MAX];
	int len;
	va_list lst;
	time_t now;


	if((log_level & level) == 0)
		return;

	// print to stdout if enabled or if log not initialised yet, i.e. log_level == 0
	if(opts.log_to_stdout || (log_level == 0 && ((LOG_INFO | LOG_ERROR) & level) != 0)){
		va_start(lst, fmt);
		vprintf(fmt, lst);
		printf("\n");
		va_end(lst);

		return;
	}

	// format text
	va_start(lst, fmt);
	len = vsnprintf(text, sizeof(text) - 1, fmt, lst);
	va_end(lst);

	// prepare memory
	if(entry->text != 0x0 && len > entry->len){
		free(entry->text);
		entry->text = 0x0;
	}

	if(entry->text == 0x0){
		entry->text = malloc(len + 1);
		entry->len = len;

		if(entry->text == 0x0){
			entry->len = 0;

			return;
		}
	}

	// populate entry
	time(&now);
	strftime(entry->time, sizeof(entry->time), "%d-%m-%Y %T%z", localtime(&now));

	strncpy(entry->text, text, len);
	entry->text[len] = 0;
	entry->level = level;

	// update log indices
	INC_IDX(log_end, 1);

	if(log_end == log_start){
		INC_IDX(log_start, 1);
		log_next = log_start;
	}

	render_mark();
}

log_entry_t *log_cycle(size_t max){
	log_entry_t *entry;
	size_t log_size;


	// first cycle
	if(log_next == log_start){
		log_size = (log_start <= log_end) ? log_end - log_start : CONFIG_LOG_MAX - log_start + log_end;

		if(log_size > max)
			INC_IDX(log_next, log_size - max);
	}

	entry = log + log_next;

	// last cycle
	if(log_next == log_end){
		log_next = log_start;

		return 0x0;
	}

	INC_IDX(log_next, 1);

	return entry;
}
