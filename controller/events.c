#include <config/config.h>
#include <X11/XKBlib.h>
#include <controller/log.h>
#include <controller/render.h>
#include <controller/uart.h>
#include <controller/xlib.h>
#include <protocol.h>


/* local/static prototypes */
static int client_message(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int configure_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int enter_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int unmap_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int expose(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int key(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int button(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);
static int motion_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart);

static uint8_t translate_keysym(KeySym sym);


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

static int (*handler[LASTEvent])(xevent_t *, xlib_obj_t *, uart_t *) = {
	[ClientMessage] = client_message,
	[ConfigureNotify] = configure_notify,
	[EnterNotify] = enter_notify,
	[UnmapNotify] = unmap_notify,
	[Expose] = expose,
	[KeyPress] = key,
	[KeyRelease] = key,
	[ButtonPress] = button,
	[ButtonRelease] = button,
	[MotionNotify] = motion_notify,
};


/* global functions */
int event_handle(xevent_t *ev, xlib_obj_t *xobj, uart_t *uart){
	DEBUG("xlib event: type=%d, name=%s, has-handler=%d", ev->type, ev_names[ev->type], (handler[ev->type] != 0x0));

	if(handler[ev->type] != 0x0)
		return handler[ev->type](ev, xobj, uart);

	return 0;
}


/* local functions */
static int client_message(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XClientMessageEvent *ev = (XClientMessageEvent*)e;
	Atom atom;


	atom = XInternAtom(xobj->dpy, "WM_DELETE_WINDOW", 0);

	if((Atom)ev->data.l[0] == atom){
		if(xobj->win != 0 && ev->window == xobj->win)
			return 1;
	}

	return 0;
}

static int configure_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XConfigureEvent *ev = &e->xconfigure;


	xlib_resize(xobj, ev->width, ev->height);
	DEBUG("resize window: width=%d, height=%d", xobj->win_width, xobj->win_height);

	return 0;
}

static int enter_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XEnterWindowEvent *ev = (XEnterWindowEvent*)e;

	xobj->cursor_x = ev->x;
	xobj->cursor_y = ev->y;

	return 0;
}

static int unmap_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	uart_stop(uart);
}

static int expose(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	render_mark();

	return 0;
}

static int key(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XKeyEvent *ev = (XKeyPressedEvent*)e;
	KeySym sym;
	uint8_t key;


	sym = XkbKeycodeToKeysym(xobj->dpy, ev->keycode, 0, 0);
	DEBUG("key %s: keycode=%u, keysym=%s", (ev->type == KeyPress) ? "press" : "release", ev->keycode, XKeysymToString(sym));

	key = translate_keysym(sym);

	if(key != 0)
		return uart_key(uart, key, (ev->type == KeyPress));

	ERROR("unsupported key: keycode=%u, keysym=%s", ev->keycode, XKeysymToString(sym));

	return -1;
}

static int button(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XButtonEvent *ev = (XButtonEvent*)e;


	DEBUG("button %s: button %d", (ev->type == ButtonPress) ? "press" : "release", ev->button);

	return uart_button(uart, ev->button, (ev->type == ButtonPress));
}

static int motion_notify(xevent_t *e, xlib_obj_t *xobj, uart_t *uart){
	XMotionEvent *ev = (XMotionEvent*)e;
	int8_t dx = ev->x - xobj->cursor_x,
		   dy = ev->y - xobj->cursor_y;


	DEBUG("mouse move: abs=(%d, %d), rel=(%d, %d)", ev->x, ev->y, dx, dy);

	xobj->cursor_x = ev->x;
	xobj->cursor_y = ev->y;
	
	return uart_move(uart, dx, dy);
}

static uint8_t translate_keysym(KeySym sym){
	/* workaround mapping to account for the blekeyboard library always using a US keyboard layout */
	switch(sym){
	case XK_y:				return 'z';		// y
	case XK_z:				return 'y';		// z
	case XK_asciicircum:	return '`';		// ^
	case XK_ssharp:			return '-';		// sz
	case XK_acute:			return '=';		// ´
	case XK_plus:			return ']';		// +
	case XK_minus:			return '/';		// -
	case XK_numbersign:		return '\\';	// #
	case XK_equal:			return '\'';	// =
	case XK_odiaeresis:		return ';';		// oe
	case XK_adiaeresis:		return '\'';	// ae
	case XK_udiaeresis:		return '[';		// ue
	}

	if(sym >= 32 && sym < 127)
		return sym;

	switch(sym){
	case XK_Control_L:			return NONASCII_BASE + 0;
	case XK_Shift_L:			return NONASCII_BASE + 1;
	case XK_Alt_L:				return NONASCII_BASE + 2;
	case XK_Super_L:			return NONASCII_BASE + 3;
	case XK_Control_R:			return NONASCII_BASE + 4;
	case XK_Shift_R:			return NONASCII_BASE + 5;
	case XK_Alt_R:				return NONASCII_BASE + 6;
	case XK_ISO_Level3_Shift:	return NONASCII_BASE + 6;
	case XK_Super_R:			return NONASCII_BASE + 7;
	case XK_Up:					return NONASCII_BASE + 8;
	case XK_Down:				return NONASCII_BASE + 9;
	case XK_Left:				return NONASCII_BASE + 10;
	case XK_Right:				return NONASCII_BASE + 11;
	case XK_BackSpace:			return NONASCII_BASE + 12;
	case XK_Tab:				return NONASCII_BASE + 13;
	case XK_Return:				return NONASCII_BASE + 14;
	case XK_Escape:				return NONASCII_BASE + 15;
	case XK_Print:				return NONASCII_BASE + 17;
	case XK_Caps_Lock:			return NONASCII_BASE + 23;
	case XK_F1:					return NONASCII_BASE + 24;
	case XK_F2:					return NONASCII_BASE + 25;
	case XK_F3:					return NONASCII_BASE + 26;
	case XK_F4:					return NONASCII_BASE + 27;
	case XK_F5:					return NONASCII_BASE + 28;
	case XK_F6:					return NONASCII_BASE + 29;
	case XK_F7:					return NONASCII_BASE + 30;
	case XK_F8:					return NONASCII_BASE + 31;
	case XK_F9:					return NONASCII_BASE + 32;
	case XK_F10:				return NONASCII_BASE + 33;
	case XK_F11:				return NONASCII_BASE + 34;
	case XK_F12:				return NONASCII_BASE + 35;
	case XK_F13:				return NONASCII_BASE + 36;
	case XK_F14:				return NONASCII_BASE + 37;
	case XK_F15:				return NONASCII_BASE + 38;
	case XK_F16:				return NONASCII_BASE + 39;
	case XK_F17:				return NONASCII_BASE + 40;
	case XK_F18:				return NONASCII_BASE + 41;
	case XK_F19:				return NONASCII_BASE + 42;
	case XK_F20:				return NONASCII_BASE + 43;
	case XK_F21:				return NONASCII_BASE + 44;
	case XK_F22:				return NONASCII_BASE + 45;
	case XK_F23:				return NONASCII_BASE + 46;
	case XK_F24:				return NONASCII_BASE + 47;
	case XK_KP_Insert:
	case XK_KP_0:				return NONASCII_BASE + 48;
	case XK_KP_End:
	case XK_KP_1:				return NONASCII_BASE + 49;
	case XK_KP_Down:
	case XK_KP_2:				return NONASCII_BASE + 50;
	case XK_KP_Page_Down:
	case XK_KP_3:				return NONASCII_BASE + 51;
	case XK_KP_Left:
	case XK_KP_4:				return NONASCII_BASE + 52;
	case XK_KP_Begin:
	case XK_KP_5:				return NONASCII_BASE + 53;
	case XK_KP_Right:
	case XK_KP_6:				return NONASCII_BASE + 54;
	case XK_KP_Home:
	case XK_KP_7:				return NONASCII_BASE + 55;
	case XK_KP_Up:
	case XK_KP_8:				return NONASCII_BASE + 56;
	case XK_KP_Page_Up:
	case XK_KP_9:				return NONASCII_BASE + 57;
	case XK_KP_Divide:			return NONASCII_BASE + 58;
	case XK_KP_Multiply:		return NONASCII_BASE + 59;
	case XK_KP_Subtract:		return NONASCII_BASE + 60;
	case XK_KP_Add:				return NONASCII_BASE + 61;
	case XK_KP_Enter:			return NONASCII_BASE + 62;
	case XK_KP_Delete:
	case XK_KP_Separator:		return NONASCII_BASE + 63;
	case XK_Num_Lock:			return NONASCII_BASE + 64;

#ifdef CONFIG_REVERSE_XKB_CUSTOM_MAP
	// Reverse effect of custom xkb file.
	//
	// The custom xkb mapping pre-translates key sequences on the xserver level, e.g. alt_l + left
	// to home. This poses a problem here since the keys sent to the target, when for instance
	// typing alt_l + left, is alt_t and home instead of alt_t and left.
	case XK_Insert:				return NONASCII_BASE + 14;
	case XK_Delete:				return NONASCII_BASE + 12;
	case XK_Page_Up:			return NONASCII_BASE + 8;
	case XK_Page_Down:			return NONASCII_BASE + 9;
	case XK_Home:				return NONASCII_BASE + 10;
	case XK_End:				return NONASCII_BASE + 11;
#else // CONFIG_REVERSE_XKB_CUSTOM
	case XK_Insert:				return NONASCII_BASE + 16;
	case XK_Delete:				return NONASCII_BASE + 18;
	case XK_Page_Up:			return NONASCII_BASE + 19;
	case XK_Page_Down:			return NONASCII_BASE + 20;
	case XK_Home:				return NONASCII_BASE + 21;
	case XK_End:				return NONASCII_BASE + 22;
#endif

	default:					return 0;
	}
}
