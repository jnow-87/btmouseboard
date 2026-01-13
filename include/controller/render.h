#ifndef RENDER_H
#define RENDER_H


#include <controller/uart.h>
#include <controller/xlib.h>


/* prototypes */
void render(xlib_obj_t *xobj, uart_t *uart);
void render_mark(void);


#endif // RENDER_H
