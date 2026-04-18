#include <config/config.h>
#include <X11/XKBlib.h>
#include <backend/backend.h>
#include <controller/log.h>
#include <controller/render.h>
#include <shared/xlib.h>


/* macros */
#define OOB_DIV			20
#define OOB_MIN_PIXEL	50

#define CURSOR_OUT_OF_BOUNDS(val, max)({ \
	typeof(val) _val = val; \
	typeof(max) _max = max; \
	typeof(max) _offset = _max / OOB_DIV; \
	\
	\
	_offset = (_offset < OOB_MIN_PIXEL) ? OOB_MIN_PIXEL : _offset; \
	(_val < _offset || _val > _max - _offset); \
})


/* local/static prototypes */
static int client_message(xevent_t *e, xlib_win_t *win, backend_t *be);
static int configure_notify(xevent_t *e, xlib_win_t *win, backend_t *be);
static int enter_notify(xevent_t *e, xlib_win_t *win, backend_t *be);
static int map_notify(xevent_t *e, xlib_win_t *win, backend_t *be);
static int unmap_notify(xevent_t *e, xlib_win_t *win, backend_t *be);
static int expose(xevent_t *e, xlib_win_t *win, backend_t *be);
static int key(xevent_t *e, xlib_win_t *win, backend_t *be);
static int button(xevent_t *e, xlib_win_t *win, backend_t *be);
static int motion_notify(xevent_t *e, xlib_win_t *win, backend_t *be);



/* static variables */
static char const *ev_names[LASTEvent] = {
	[ClientMessage] = "ClientMessage",
	[ConfigureNotify] = "ConfigureNotify",
	[EnterNotify] = "EnterNotify",
	[UnmapNotify] = "UnmapNotify",
	[Expose] = "Expose",
	[KeyPress] = "KeyPress",
	[KeyRelease] = "KeyRelease",
	[ButtonPress] = "ButtonPress",
	[ButtonRelease] = "ButtonRelease",
	[MotionNotify] = "MotionNotify",
};

static int (*handler[LASTEvent])(xevent_t *, xlib_win_t *, backend_t *) = {
	[ClientMessage] = client_message,
	[ConfigureNotify] = configure_notify,
	[EnterNotify] = enter_notify,
	[MapNotify] = map_notify,
	[UnmapNotify] = unmap_notify,
	[Expose] = expose,
	[KeyPress] = key,
	[KeyRelease] = key,
	[ButtonPress] = button,
	[ButtonRelease] = button,
	[MotionNotify] = motion_notify,
};


/* global functions */
int event_handle(xevent_t *ev, xlib_win_t *win, backend_t *be){
	DEBUG("xlib event: type=%d, name=%s, has-handler=%d", ev->type, ev_names[ev->type], (handler[ev->type] != 0x0));

	if(handler[ev->type] != 0x0)
		return handler[ev->type](ev, win, be);

	return 0;
}


/* local functions */
static int client_message(xevent_t *e, xlib_win_t *win, backend_t *be){
	XClientMessageEvent *ev = (XClientMessageEvent*)e;
	char err[128];
	Atom atom;


	if(xlib_error(win->xobj, err, sizeof(err)) == 0){
		ERROR("xlib %s", err);

		return 1;
	}

	atom = XInternAtom(win->xobj->dpy, "WM_DELETE_WINDOW", 0);

	if((Atom)ev->data.l[0] == atom){
		if(win->id != 0 && ev->window == win->id)
			return 1;
	}

	return 0;
}

static int configure_notify(xevent_t *e, xlib_win_t *win, backend_t *be){
	XConfigureEvent *ev = &e->xconfigure;


	xlib_resize(win, ev->width, ev->height);
	DEBUG("resize window: width=%d, height=%d", win->width, win->height);

	return 0;
}

static int enter_notify(xevent_t *e, xlib_win_t *win, backend_t *be){
	XEnterWindowEvent *ev = (XEnterWindowEvent*)e;

	win->cursor_x = ev->x;
	win->cursor_y = ev->y;

	return 0;
}

static int map_notify(xevent_t *e, xlib_win_t *win, backend_t *be){
	xlib_cursor_visible(win, false);

	return 0;
}

static int unmap_notify(xevent_t *e, xlib_win_t *win, backend_t *be){
	xlib_cursor_visible(win, true);
	be->stop(be);

	return 0;
}

static int expose(xevent_t *e, xlib_win_t *win, backend_t *be){
	render_mark();

	return 0;
}

static int key(xevent_t *e, xlib_win_t *win, backend_t *be){
	XKeyEvent *ev = (XKeyPressedEvent*)e;
	KeySym sym;
	uint8_t key;


	sym = XkbKeycodeToKeysym(win->xobj->dpy, ev->keycode, 0, 0);
	DEBUG("key %s: keycode=%u, keysym=%s", (ev->type == KeyPress) ? "press" : "release", ev->keycode, XKeysymToString(sym));

	return be->key(be, sym, (ev->type == KeyPress));
}

static int button(xevent_t *e, xlib_win_t *win, backend_t *be){
	XButtonEvent *ev = (XButtonEvent*)e;


	DEBUG("button %s: button %d", (ev->type == ButtonPress) ? "press" : "release", ev->button);

	return be->button(be, ev->button, (ev->type == ButtonPress));
}

static int motion_notify(xevent_t *e, xlib_win_t *win, backend_t *be){
	XMotionEvent *ev = (XMotionEvent*)e;
	int8_t dx = ev->x - win->cursor_x,
		   dy = ev->y - win->cursor_y;


	DEBUG("mouse move: abs=(%d, %d), rel=(%d, %d)", ev->x, ev->y, dx, dy);

	win->cursor_x = ev->x;
	win->cursor_y = ev->y;

	/* reset the cursor to the window center if it goes out of a certain area
	 *  moving the cursor via xlib also causes XMotionEvent events, those events
	 *  must not be translated to the receiver, hence the window cursor position
	 *  is updated, hence dx and dy are zero for the upcoming XMotionEvents
	 */
	if(CURSOR_OUT_OF_BOUNDS(win->cursor_x, win->width))
		win->cursor_x = win->width / 2;

	if(CURSOR_OUT_OF_BOUNDS(win->cursor_y, win->height))
		win->cursor_y = win->height / 2;

	if(win->cursor_x != ev->x || win->cursor_y != ev->y)
		xlib_cursor_move(win, win->cursor_x, win->cursor_y);

	return be->move(be, dx, dy);
}
