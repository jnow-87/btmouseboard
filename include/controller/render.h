#ifndef RENDER_H
#define RENDER_H


#include <backend/backend.h>
#include <shared/xlib.h>


/* prototypes */
void render(xlib_win_t *win, backend_t *be);
void render_mark(void);


#endif // RENDER_H
