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
	COLOR_SUCCESS,
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
} xlib_obj_t;

typedef struct{
	xlib_obj_t *xobj;

	int width,
		height;
	Window id;

	gfx_t *gfx;

	int cursor_x,
		cursor_y;
} xlib_win_t;


/* prototypes */
xlib_obj_t *xlib_init(void);
void xlib_destroy(xlib_obj_t *xobj);

xlib_win_t *xlib_win_create(xlib_obj_t *xobj, char *win_class_name);
void xlib_win_destroy(xlib_win_t *win);

int xlib_event(xlib_obj_t *xobj, xevent_t *ev);
int xlib_error(xlib_obj_t *xobj, char *s, int n);
void xlib_resize(xlib_win_t *win, int width, int height);

void xlib_scene_begin(xlib_win_t *win);
void xlib_scene_end(xlib_win_t *win);
void xlib_sync(xlib_obj_t *xobj);

unsigned int xlib_printf(xlib_win_t *win, int x, int y, char const *fmt, ...);
unsigned int xlib_cprintf(xlib_win_t *win, int x, int y, color_t color, char const *fmt, ...);
unsigned int xlib_cdprintf(xlib_win_t *win, int x, int y, color_t color, char const *fmt, va_list lst);
void xlib_rect(xlib_win_t *win, int x, int y, unsigned int width, unsigned int height, color_t color, bool filled);

int xlib_cursor_move(xlib_obj_t *xobj, int dx, int dy);
int xlib_cursor_move_to(xlib_win_t *win, int x, int y);
int xlib_cursor_click(xlib_obj_t *xobj, int button, bool press);
void xlib_cursor_visible(xlib_win_t *win, bool visible);

int  xlib_key(xlib_obj_t *xobj, KeySym sym, bool press);


#endif // XLIB_H
