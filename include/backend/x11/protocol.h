#ifndef X11_PROTOCOL_H
#define X11_PROTOCOL_H


#include <stdint.h>
#include <X11/X.h>


/* types */
typedef enum{
	CMD_KEY = 1,
	CMD_BUTTON,
	CMD_MOVE,
	CMD_MAX
} cmd_t;

typedef enum : int8_t{
	RESP_ECMD = -2,
	RESP_EXLIB = -1,
	RESP_OK = 0,
} response_t;

typedef struct{
	cmd_t cmd;

	KeySym sym;
	uint8_t button;

	bool pressed;
	int8_t dx,
		   dy;
} dgram_t;


#endif // X11_PROTOCOL_H
