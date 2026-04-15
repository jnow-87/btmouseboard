#ifndef HARDWARE_H
#define HARDWARE_H


#include <stdint.h>
#include <shared/xlib.h>
#include <X11/X.h>


/* types */
typedef struct backend_t{
	void *data;

	// callbacks
	void (*destroy)(struct backend_t *be);

	int (*stop)(struct backend_t *be);

	int (*key)(struct backend_t *be, KeySym sym, bool press);
	int (*button)(struct backend_t *be, uint8_t button, bool press);
	int (*move)(struct backend_t *be, int8_t dx, int8_t dy);

	unsigned int (*render_status)(struct backend_t *be, xlib_obj_t *xobj, unsigned int x, unsigned int y);
} backend_t;


/* prototypes */
backend_t *backend_create_uart(void);


#endif // HARDWARE_H
