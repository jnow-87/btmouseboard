#ifndef PROTOCOL_H
#define PROTOCOL_H


#include <stdint.h>


/* macros */
#define NONASCII_BASE	127


/* types */
typedef enum : uint8_t{
	HDR_PING = 1,
	HDR_CLOSE,
	HDR_KEY_PRESS,
	HDR_KEY_RELEASE,
	HDR_BUTTON_PRESS,
	HDR_BUTTON_RELEASE,
	HDR_VSCROLL,
	HDR_HSCROLL,
	HDR_MOVE,
	HDR_MAX
} hdr_t;

typedef enum : int8_t{
	RESP_EINVAL_KEY = -3,
	RESP_EINVAL_CMD = -2,
	RESP_ENOCON = -1,
	RESP_OK = 0,
	RESP_MAGIC = 0x42,
} response_t;


#endif // PROTOCOL_H
