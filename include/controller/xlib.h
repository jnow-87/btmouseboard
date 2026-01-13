#ifndef XLIB_H
#define XLIB_H


#include <stdarg.h>
#include <stdbool.h>
#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>


/* types */
typedef XEvent xevent_t;

typedef enum{
	COLOR_NONE = -1,
	COLOR_TEXT = 0,
	COLOR_ERROR,
	COLOR_INFO,
	COLOR_BACKGROUND,
	COLOR_STATUSLINE,
	COLOR_BLUETOOTH,
	COLOR_MAX
} color_t;

typedef struct{
	Drawable drawable;
	XftDraw *xft_drawable;
	GC gc;

	XftFont *font;
	int font_height;

	XftColor colors[COLOR_MAX];
} gfx_t;

typedef struct{
	Display *dpy;
	Window root;
	int screen;

	int win_width,
		win_height;
	Window win;

	gfx_t *gfx;

	int cursor_x,
		cursor_y;
} xlib_obj_t;


/* prototypes */
xlib_obj_t *xlib_init(char *win_class_name);
void xlib_destroy(xlib_obj_t *xobj);

int xlib_event(xlib_obj_t *xobj, xevent_t *ev);
void xlib_resize(xlib_obj_t *xobj, int width, int height);

void xlib_scene_begin(xlib_obj_t *xobj);
void xlib_scene_end(xlib_obj_t *xobj);

unsigned int xlib_printf(xlib_obj_t *xobj, int x, int y, char const *fmt, ...);
unsigned int xlib_cprintf(xlib_obj_t *xobj, int x, int y, color_t color, char const *fmt, ...);
unsigned int xlib_cdprintf(xlib_obj_t *xobj, int x, int y, color_t color, char const *fmt, va_list lst);
void xlib_rect(xlib_obj_t *xobj, int x, int y, unsigned int width, unsigned int height, color_t color, bool filled);


#endif // XLIB_H
