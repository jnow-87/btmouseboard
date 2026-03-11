#ifndef LOG_H
#define LOG_H


#include <stdarg.h>


/* macros */
#define ERROR(fmt, ...)		({ log_msg(LOG_ERROR, "%s:%d:\033[31merror\033[0m: " fmt, __FILE__, __LINE__, ##__VA_ARGS__); -1; })
#define INFO(fmt, ...)		log_msg(LOG_INFO, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define DEBUG(fmt, ...)		log_msg(LOG_DEBUG, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)


/* types */
typedef enum{
	LOG_ERROR = 1,
	LOG_INFO,
	LOG_DEBUG
} log_level_t;


/* prototypes */
void log_msg(log_level_t level, char const *fmt, ...);


#endif // LOG_H
