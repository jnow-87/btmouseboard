#include <config/config.h>
#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <shared/xlib.h>


/* local/static prototypes */
static gfx_t *gfx_init(xlib_obj_t *xobj, int width, int height);
static void gfx_destroy(gfx_t *gfx, xlib_obj_t *xobj);

static int error_handler(Display *dpy, XErrorEvent *evt);


/* static variables */
static unsigned char xerrno = 0;


/* global functions */
xlib_obj_t *xlib_init(void){
	char *display;
	xlib_obj_t *xobj;


	xobj = calloc(1, sizeof(xlib_obj_t));

	if(xobj == 0x0)
		goto err_0;

	XSetErrorHandler(error_handler);

	display = getenv("DISPLAY");
	xobj->dpy = XOpenDisplay((display != 0x0) ? display : ":0.0");

	if(xobj->dpy == 0x0)
		goto err_1;

	xobj->screen = DefaultScreen(xobj->dpy);
	xobj->root = RootWindow(xobj->dpy, xobj->screen);

	return xobj;


err_1:
	free(xobj);

err_0:
	return 0x0;
}

void xlib_destroy(xlib_obj_t *xobj){
	XCloseDisplay(xobj->dpy);
	free(xobj);
}

xlib_win_t *xlib_win_create(xlib_obj_t *xobj, char *win_class_name){
	xlib_win_t *win;
	Atom atom;


	win = malloc(sizeof(xlib_win_t));

	if(win == 0x0)
		goto err_0;

	win->xobj = xobj;
	win->width = CONFIG_WIN_WIDTH;
	win->height = CONFIG_WIN_HEIGHT;

	win->gfx = gfx_init(xobj, win->height, win->height);

	if(win->gfx == 0x0)
		goto err_1;

	win->id = XCreateWindow(
		xobj->dpy,
		xobj->root,
		100,
		100,
		win->width,
		win->height,
		0,
		DefaultDepth(xobj->dpy, xobj->screen),
		CopyFromParent,
		DefaultVisual(xobj->dpy, xobj->screen),
		CWOverrideRedirect | CWBackPixmap | CWEventMask,
		&(XSetWindowAttributes){
			.background_pixmap = ParentRelative,
			.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | PointerMotionMask | EnterWindowMask
		}
	);

	XSetClassHint(xobj->dpy, win->id, &(XClassHint){win_class_name, win_class_name});
	XMapWindow(xobj->dpy, win->id);

	// enable destroy detection
	atom = XInternAtom(xobj->dpy, "WM_DELETE_WINDOW", 1);

	if(atom == None || XSetWMProtocols(xobj->dpy, win->id, &atom, 1) == 0)
		goto err_1;

	return win;


err_1:
	xlib_win_destroy(win);

err_0:
	return 0x0;
}

void xlib_win_destroy(xlib_win_t *win){
	XDestroyWindow(win->xobj->dpy, win->id);

	if(win->gfx != 0x0)
		gfx_destroy(win->gfx, win->xobj);

	free(win);
}

int xlib_event(xlib_obj_t *xobj, xevent_t *ev){
	if(xerrno != 0)
		return -1;

	if(XNextEvent(xobj->dpy, ev))
		return -1;

	return 0;
}

int xlib_error(xlib_obj_t *xobj, char *s, int n){
	if(xerrno == 0)
		return -1;

	XGetErrorText(xobj->dpy, xerrno, s, n);
	s[n - 1] = 0;
	xerrno = 0;

	return 0;
}

void xlib_resize(xlib_win_t *win, int width, int height){
	int screen = win->xobj->screen;
	Display *dpy = win->xobj->dpy;
	Window root = win->xobj->root;
	gfx_t *gfx = win->gfx;


	if(gfx->drawable)
		XFreePixmap(dpy, gfx->drawable);

	if(gfx->xft_drawable)
		XftDrawDestroy(gfx->xft_drawable);

	gfx->drawable = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, screen));
	gfx->xft_drawable = XftDrawCreate(dpy, gfx->drawable, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));

	win->width = width;
	win->height = height;
}

void xlib_scene_begin(xlib_win_t *win){
	gfx_t *gfx = win->gfx;


	XSetForeground(win->xobj->dpy, gfx->gc, gfx->colors[COLOR_BACKGROUND].pixel);
	XFillRectangle(win->xobj->dpy, gfx->drawable, gfx->gc, 0, 0, win->width, win->height);
}

void xlib_scene_end(xlib_win_t *win){
	XCopyArea(win->xobj->dpy, win->gfx->drawable, win->id, win->gfx->gc, 0, 0, win->width, win->height, 0, 0);
	xlib_sync(win->xobj);
}

void xlib_sync(xlib_obj_t *xobj){
	XSync(xobj->dpy, False);
}

unsigned int xlib_printf(xlib_win_t *win, int x, int y, char const *fmt, ...){
	unsigned int len;
	va_list lst;


	va_start(lst, fmt);
	len = xlib_cdprintf(win, x, y, COLOR_TEXT, fmt, lst);
	va_end(lst);

	return len;
}

unsigned int xlib_cprintf(xlib_win_t *win, int x, int y, color_t color, char const *fmt, ...){
	unsigned int len;
	va_list lst;


	va_start(lst, fmt);
	len = xlib_cdprintf(win, x, y, color, fmt, lst);
	va_end(lst);

	return len;
}

unsigned int xlib_cdprintf(xlib_win_t *win, int x, int y, color_t color, char const *fmt, va_list lst){
	gfx_t *gfx = win->gfx;
	char s[LINE_MAX];
	int len;
	XGlyphInfo ext;


	len = vsnprintf(s, sizeof(s), fmt, lst);
	s[sizeof(s) - 1] = 0;

	XftDrawStringUtf8(gfx->xft_drawable, &gfx->colors[color], gfx->font, x, y + gfx->font_height, (XftChar8*)s, len);
	XftTextExtentsUtf8(win->xobj->dpy, gfx->font, (XftChar8*)s, len, &ext);

	return ext.xOff;
}

void xlib_rect(xlib_win_t *win, int x, int y, unsigned int width, unsigned int height, color_t color, bool filled){
	xlib_obj_t *xobj = win->xobj;
	gfx_t *gfx = win->gfx;


	XSetForeground(xobj->dpy, gfx->gc, gfx->colors[color].pixel);

	if(filled)	XFillRectangle(xobj->dpy, gfx->drawable, gfx->gc, x, y, width, height);
	else		XDrawRectangle(xobj->dpy, gfx->drawable, gfx->gc, x, y, width - 1, height - 1);
}

int xlib_cursor_move(xlib_obj_t *xobj, int dx, int dy){
	return (XWarpPointer(xobj->dpy, None, None, 0, 0, 0, 0, dx, dy) == 0) ? -1 : 0;
}

int xlib_cursor_move_to(xlib_win_t *win, int x, int y){
	return (XWarpPointer(win->xobj->dpy, None, win->id, 0, 0, 0, 0, x, y) == 0) ? -1 : 0;
}

int xlib_cursor_click(xlib_obj_t *xobj, int button, bool press){
	return (XTestFakeButtonEvent(xobj->dpy, button, press, CurrentTime) == 0) ? -1 : 0;
}

void xlib_cursor_visible(xlib_win_t *win, bool visible){
	xlib_obj_t *xobj = win->xobj;


	if(visible)		XFixesShowCursor(xobj->dpy, win->id);
	else			XFixesHideCursor(xobj->dpy, xobj->root);
}

int xlib_key(xlib_obj_t *xobj, KeySym sym, bool press){
	return (XTestFakeKeyEvent(xobj->dpy, XKeysymToKeycode(xobj->dpy, sym), press, CurrentTime) == 0) ? -1 : 0;
}


/* local functions */
static gfx_t *gfx_init(xlib_obj_t *xobj, int width, int height){
	char const *color_names[] = {
		CONFIG_COLOR_TEXT,
		CONFIG_COLOR_ERROR,
		CONFIG_COLOR_INFO,
		CONFIG_COLOR_SUCCESS,
		CONFIG_COLOR_BACKGROUND,
		CONFIG_COLOR_STATUSLINE,
		CONFIG_COLOR_BLUETOOTH,
	};
	int screen = xobj->screen;
	Display *dpy = xobj->dpy;
	Window root = xobj->root;
	Visual *visual = DefaultVisual(xobj->dpy, xobj->screen);
	Colormap colmap = DefaultColormap(xobj->dpy, xobj->screen);
	gfx_t *gfx;


	/* init drawable */
	gfx = calloc(1, sizeof(gfx_t));

	if(gfx == 0x0)
		goto err_0;

	gfx->drawable = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, screen));
	gfx->xft_drawable = XftDrawCreate(dpy, gfx->drawable, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));

	gfx->gc = XCreateGC(dpy, root, 0, 0x0);
	XSetLineAttributes(dpy, gfx->gc, 1, LineSolid, CapButt, JoinMiter);

	// avoid getting NoExpose events on XCopyArea()
	XChangeGC(dpy, gfx->gc, GCGraphicsExposures, &(XGCValues){ .graphics_exposures = false });

	/* init font */
	gfx->font = XftFontOpenName(xobj->dpy, xobj->screen, CONFIG_FONT);

	if(gfx->font == 0x0)
		goto err_1;

	gfx->font_height = gfx->font->ascent + gfx->font->descent;

	/* init colors */
	for(size_t i=0; i<COLOR_MAX; i++){
		if(!XftColorAllocName(xobj->dpy, visual, colmap, color_names[i], gfx->colors + i))
			goto err_1;
	}

	return gfx;


err_1:
	gfx_destroy(gfx, xobj);

err_0:
	return 0x0;
}

static void gfx_destroy(gfx_t *gfx, xlib_obj_t *xobj){
	for(size_t i=0; i<COLOR_MAX; i++)
		XftColorFree(xobj->dpy, DefaultVisual(xobj->dpy, xobj->screen), DefaultColormap(xobj->dpy, xobj->screen), gfx->colors + i);

	if(gfx->font)
		XftFontClose(xobj->dpy, gfx->font);

	XFreeGC(xobj->dpy, gfx->gc);
	XftDrawDestroy(gfx->xft_drawable);
	XFreePixmap(xobj->dpy, gfx->drawable);

	free(gfx);
}

static int error_handler(Display *dpy, XErrorEvent *evt){
	xerrno = evt->error_code;

	return 0;
}
