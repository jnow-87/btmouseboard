#ifndef EVENTS_H
#define EVENTS_H


#include <controller/uart.h>
#include <controller/xlib.h>


/* prototypes */
int event_handle(xevent_t *ev, xlib_obj_t *xobj, uart_t *uart);


#endif // EVENTS_H
