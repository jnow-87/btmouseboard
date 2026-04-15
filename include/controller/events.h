#ifndef EVENTS_H
#define EVENTS_H


#include <backend/backend.h>
#include <shared/xlib.h>


/* prototypes */
int event_handle(xevent_t *ev, xlib_obj_t *xobj, backend_t *be);


#endif // EVENTS_H
