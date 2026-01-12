#include <config/config.h>
#include <X11/X.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <controller/log.h>
#include <controller/xlib.h>


/* local/static prototypes */
static Window win_create(char *win_class_name, xlib_obj_t *xobj);

static gfx_t *gfx_init(xlib_obj_t *xobj);
static void gfx_destroy(gfx_t *gfx, xlib_obj_t *xobj);

static int error_handler(Display *dsp, XErrorEvent *evt);


/* static variables */
static unsigned char xerrno = 0;


/* global functions */
xlib_obj_t *xlib_init(char *win_class_name){
	xlib_obj_t *xobj;


	xobj = calloc(1, sizeof(xlib_obj_t));

	if(xobj == 0x0)
		goto err_0;

	XSetErrorHandler(error_handler);
	xobj->dpy = XOpenDisplay(0x0);

	if(xobj->dpy == 0x0)
		goto err_1;

	xobj->win_width = CONFIG_WIN_WIDTH;
	xobj->win_height = CONFIG_WIN_HEIGHT;
	xobj->screen = DefaultScreen(xobj->dpy);
	xobj->root = RootWindow(xobj->dpy, xobj->screen);

	xobj->gfx = gfx_init(xobj);

	if(xobj->gfx == 0x0)
		goto err_1;

	xobj->win = win_create(win_class_name, xobj);

	if(xobj->win == None)
		goto err_1;

	return xobj;


err_1:
	xlib_destroy(xobj);

err_0:
	return 0x0;
}

void xlib_destroy(xlib_obj_t *xobj){
	gfx_destroy(xobj->gfx, xobj);
	XDestroyWindow(xobj->dpy, xobj->win);
	XCloseDisplay(xobj->dpy);
	free(xobj);
}

int xlib_event(xlib_obj_t *xobj, xevent_t *ev){
	if(xerrno != 0)
		return -1;

	if(XNextEvent(xobj->dpy, ev))
		return -1;

	return 0;
}

void xlib_resize(xlib_obj_t *xobj, int width, int height){
	int screen = xobj->screen;
	Display *dpy = xobj->dpy;
	Window root = xobj->root;
	gfx_t *gfx = xobj->gfx;


	if(gfx->drawable)
		XFreePixmap(dpy, gfx->drawable);

	if(gfx->xft_drawable)
		XftDrawDestroy(gfx->xft_drawable);

	gfx->drawable = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, screen));
	gfx->xft_drawable = XftDrawCreate(dpy, gfx->drawable, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));

	xobj->win_width = width;
	xobj->win_height = height;
}

void xlib_scene_begin(xlib_obj_t *xobj){
	gfx_t *gfx = xobj->gfx;


	XSetForeground(xobj->dpy, gfx->gc, gfx->colors[COLOR_BACKGROUND].pixel);
	XFillRectangle(xobj->dpy, gfx->drawable, gfx->gc, 0, 0, xobj->win_width, xobj->win_height);
}

void xlib_scene_end(xlib_obj_t *xobj){
	XCopyArea(xobj->dpy, xobj->gfx->drawable, xobj->win, xobj->gfx->gc, 0, 0, xobj->win_width, xobj->win_height, 0, 0);
	XSync(xobj->dpy, False);
}

unsigned int xlib_printf(xlib_obj_t *xobj, int x, int y, char const *fmt, ...){
	unsigned int len;
	va_list lst;


	va_start(lst, fmt);
	len = xlib_cdprintf(xobj, x, y, COLOR_TEXT, fmt, lst);
	va_end(lst);

	return len;
}

unsigned int xlib_cprintf(xlib_obj_t *xobj, int x, int y, color_t color, char const *fmt, ...){
	unsigned int len;
	va_list lst;


	va_start(lst, fmt);
	len = xlib_cdprintf(xobj, x, y, color, fmt, lst);
	va_end(lst);

	return len;
}

unsigned int xlib_cdprintf(xlib_obj_t *xobj, int x, int y, color_t color, char const *fmt, va_list lst){
	gfx_t *gfx = xobj->gfx;
	char s[LINE_MAX];
	int len;
	XGlyphInfo ext;


	len = vsnprintf(s, sizeof(s), fmt, lst);
	s[sizeof(s) - 1] = 0;

	XftDrawStringUtf8(gfx->xft_drawable, &gfx->colors[color], gfx->font, x, y + gfx->font_height, (XftChar8*)s, len);
	XftTextExtentsUtf8(xobj->dpy, gfx->font, (XftChar8*)s, len, &ext);

	return ext.xOff;
}

void xlib_rect(xlib_obj_t *xobj, int x, int y, unsigned int width, unsigned int height, color_t color, bool filled){
	gfx_t *gfx = xobj->gfx;


	XSetForeground(xobj->dpy, gfx->gc, gfx->colors[color].pixel);

	if(filled)	XFillRectangle(xobj->dpy, gfx->drawable, gfx->gc, x, y, width, height);
	else		XDrawRectangle(xobj->dpy, gfx->drawable, gfx->gc, x, y, width - 1, height - 1);
}


/* local functions */
static Window win_create(char *win_class_name, xlib_obj_t *xobj){
	Window win;


	win = XCreateWindow(
		xobj->dpy,
		xobj->root,
		100,
		100,
		xobj->win_width,
		xobj->win_height,
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

	XSetClassHint(xobj->dpy, win, &(XClassHint){win_class_name, win_class_name});
	XMapWindow(xobj->dpy, win);

	// enable destroy detection
	Atom atom;
	atom = XInternAtom(xobj->dpy, "WM_DELETE_WINDOW", 1);

	if(atom == None || XSetWMProtocols(xobj->dpy, win, &atom, 1) == 0)
		return None;

	return win;
}

static gfx_t *gfx_init(xlib_obj_t *xobj){
	char const *color_names[] = {
		CONFIG_COLOR_TEXT,
		CONFIG_COLOR_ERROR,
		CONFIG_COLOR_INFO,
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

	gfx->drawable = XCreatePixmap(dpy, root, xobj->win_width, xobj->win_height, DefaultDepth(dpy, screen));
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

static int error_handler(Display *dsp, XErrorEvent *evt){
	char msg[128];


	XGetErrorText(dsp, evt->error_code, msg, sizeof(msg));
	msg[sizeof(msg) - 1] = 0;

	ERROR("xlib %s", msg);
	xerrno = evt->error_code;

	return 0;
}
