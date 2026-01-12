#ifndef LOG_H
#define LOG_H


#include <stdbool.h>
#include <stddef.h>


/* macros */
#define ERROR(fmt, ...)		({ log_add(LOG_ERROR, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__); -1; })
#define INFO(fmt, ...)		log_add(LOG_INFO, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG(fmt, ...)		log_add(LOG_DEBUG, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)


/* types */
typedef enum{
	LOG_INFO = 0x1,
	LOG_DEBUG = 0x2,
	LOG_ERROR = 0x4,
} log_level_t;

typedef struct{
	log_level_t level;
	char time[25];
	char *text;
	size_t len;
} log_entry_t;


/* prototypes */
int log_init(bool debug);
void log_add(log_level_t level, char const *fmt, ...);
log_entry_t *log_cycle(size_t max);


#endif // LOG_H
