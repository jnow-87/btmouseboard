#include <config/config.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <X11/X.h>
#include <backend/backend.h>
#include <backend/bluetooth/protocol.h>
#include <controller/log.h>
#include <controller/opts.h>
#include <controller/render.h>
#include <shared/xlib.h>


/* macros */
#define _BRATE(x)				B##x
#define TERMIOS_BRATE(baud)		_BRATE(baud)

/* types */
typedef struct{
	int fd;
	unsigned int dev_num;
	bool connected;
} uart_t;


/* local/static prototypes */
// backend callbacks
static void be_destroy(backend_t *be);

static int be_stop(backend_t *be);

static int be_key(backend_t *be, KeySym sym, bool press);
static int be_button(backend_t *be, uint8_t button, bool press);
static int be_move(backend_t *be, int8_t dx, int8_t dy);

static unsigned int be_render_status(backend_t *be, xlib_win_t *win, unsigned int x, unsigned int y);

// firmware protocol
static int hscroll(uart_t *uart, uint8_t button);
static int vscroll(uart_t *uart, uint8_t button);

static int send_cmd(uart_t *uart, hdr_t hdr, uint8_t *data, size_t ndata);
static int trywrite(uart_t *uart, uint8_t *data, size_t n);

// uart device handling
static void reinit(uart_t *uart);
static void discover(uart_t *uart, char const *fmt);
static int configure(int fd);

// helper
static uint8_t translate_keysym(KeySym sym);

static char const *strcmd(hdr_t hdr);
static char const *strresp(response_t resp);


/* global functions */
backend_t *backend_create_uart(void){
	backend_t *be;


	be = malloc(sizeof(backend_t));

	if(be == 0x0)
		goto err_0;

	be->data = malloc(sizeof(uart_t));

	if(be->data == 0x0)
		goto err_1;

	be->destroy = be_destroy;
	be->stop = be_stop;
	be->key = be_key;
	be->button = be_button;
	be->move = be_move;
	be->render_status = be_render_status;

	((uart_t*)be->data)->fd = -1;
	reinit(be->data);

	return be;


err_1:
	free(be);

err_0:
	ERROR("allocating bluetooth backend");

	return 0x0;
}


/* local functions */
static void be_destroy(backend_t *be){
	uart_t *uart = (uart_t*)be->data;


	if(uart->fd >= 0){
		be_stop(be);
		close(uart->fd);
	}

	free(uart);
	free(be);
}

static int be_stop(backend_t *be){
	return send_cmd(be->data, HDR_CLOSE, 0x0, 0);
}

static int be_key(backend_t *be, KeySym sym, bool press){
	uint8_t key;


	key = translate_keysym(sym);

	if(key == 0)
		return ERROR("unsupported key: keysym=%s", XKeysymToString(sym));

	return send_cmd(be->data, press ? HDR_KEY_PRESS : HDR_KEY_RELEASE, &key, 1);
}

static int be_button(backend_t *be, uint8_t button, bool press){
	if(button == 4 || button == 5)
		return vscroll(be->data, button);

	if(button == 6 || button == 7)
		return hscroll(be->data, button);

	return send_cmd(be->data, press ? HDR_BUTTON_PRESS : HDR_BUTTON_RELEASE, &button, 1);
}

static int be_move(backend_t *be, int8_t dx, int8_t dy){
	return send_cmd(be->data, HDR_MOVE, (uint8_t []){dx, dy}, 2);
}

static unsigned int be_render_status(backend_t *be, xlib_win_t *win, unsigned int x, unsigned int y){
	unsigned int dx = 0;
	uart_t *uart = (uart_t*)be->data;


	dx += xlib_cprintf(win, x, y, uart->connected ? COLOR_BLUETOOTH : COLOR_TEXT, "   ");
	dx += xlib_cprintf(win, x + dx, y, COLOR_TEXT, (uart->fd >= 0) ? CONFIG_UART_PATTERN : "none", uart->dev_num);

	return dx;
}

static int hscroll(uart_t *uart, uint8_t button){
	return send_cmd(uart, HDR_HSCROLL, (uint8_t []){ (button == 7) ? CONFIG_SCROLL_DISTANCE : -CONFIG_SCROLL_DISTANCE }, 1);
}

static int vscroll(uart_t *uart, uint8_t button){
	return send_cmd(uart, HDR_VSCROLL, (uint8_t []){ (button == 4) ? CONFIG_SCROLL_DISTANCE : -CONFIG_SCROLL_DISTANCE }, 1);
}

static int send_cmd(uart_t *uart, hdr_t hdr, uint8_t *data, size_t ndata){
	response_t resp = RESP_ENOCON;


	if(trywrite(uart, &hdr, 1) != 0)
		return RESP_ENOCON;

	if(ndata && data){
		if(trywrite(uart, data, ndata) != 0)
			return RESP_ENOCON;
	}

	read(uart->fd, &resp, 1);

	if(uart->connected != (resp != RESP_ENOCON))
		render_mark();

	uart->connected = (resp != RESP_ENOCON);

	DEBUG("send command %s: %s", strcmd(hdr), strresp(resp));

	return -(resp != RESP_OK);
}

static int trywrite(uart_t *uart, uint8_t *data, size_t n){
	if(write(uart->fd, data, n) == n)
		return 0;

	reinit(uart);

	return -1;
}

static void reinit(uart_t *uart){
	if(uart->fd >= 0){
		close(uart->fd);
		render_mark();
	}

	uart->fd = -1;
	uart->connected = false;

	discover(uart, CONFIG_UART_PATTERN);
}

static void discover(uart_t *uart, char const *fmt){
	int fd;
	uint8_t resp;
	char dev[strlen(fmt) + 1];


	for(unsigned int i=0; i<10; i++){
		snprintf(dev, sizeof(dev), fmt, i);
		dev[sizeof(dev)] = 0;

		fd = open(dev, O_RDWR);

		if(fd < 0)
			continue;

		DEBUG("ping device %s", dev);

		if(configure(fd) != 0)
			goto err;
			
		if(write(fd, (uint8_t []){ HDR_PING }, 1) != 1)
			goto err;
		
		if(read(fd, &resp, 1) == 1 && resp == RESP_MAGIC){
			DEBUG("device found at %s", dev);
			render_mark();

			uart->fd = fd;
			uart->dev_num = i;

			break;
		}

		DEBUG("received no or invalid response");

err:
		close(fd);
	}
}

static int configure(int fd){
	struct termios attr;


	if(tcgetattr(fd, &attr) != 0)
		return -1;

	attr.c_iflag = 0;
	attr.c_oflag = 0;
	attr.c_lflag = 0;

	attr.c_cflag = CBAUDEX | CLOCAL | HUPCL | CREAD | CS8;

	// enable read timeout
	attr.c_lflag &= ~ICANON;	// non-canonical mode
	attr.c_cc[VMIN] = 0;		// min chars for read
	attr.c_cc[VTIME] = 5;		// read timeout in deciseconds

	if(cfsetspeed(&attr, TERMIOS_BRATE(CONFIG_UART_BAUDRATE)) != 0)
		return -1;

	if(tcsetattr(fd, TCSANOW, &attr) != 0)
		return -1;

	return 0;
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

	if(opts.reverse_custom_xkb_map){
		// Reverse effect of custom xkb file.
		//
		// The custom xkb mapping pre-translates key sequences on the xserver level, e.g. alt_l + left
		// to home. This poses a problem here since the keys sent to the target, when for instance
		// typing alt_l + left, is alt_t and home instead of alt_t and left.
		switch(sym){
		case XK_Insert:				return NONASCII_BASE + 14;
		case XK_Delete:				return NONASCII_BASE + 12;
		case XK_Page_Up:			return NONASCII_BASE + 8;
		case XK_Page_Down:			return NONASCII_BASE + 9;
		case XK_Home:				return NONASCII_BASE + 10;
		case XK_End:				return NONASCII_BASE + 11;
		default:					break;
		}
	}

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
	case XK_Insert:				return NONASCII_BASE + 16;
	case XK_Delete:				return NONASCII_BASE + 18;
	case XK_Page_Up:			return NONASCII_BASE + 19;
	case XK_Page_Down:			return NONASCII_BASE + 20;
	case XK_Home:				return NONASCII_BASE + 21;
	case XK_End:				return NONASCII_BASE + 22;
	default:					return 0;
	}
}

static char const *strcmd(hdr_t hdr){
	switch(hdr){
	case HDR_PING:				return "ping";
	case HDR_CLOSE:				return "close";
	case HDR_KEY_PRESS:			return "key-press";
	case HDR_KEY_RELEASE:		return "key-release";
	case HDR_BUTTON_PRESS:		return "button-press";
	case HDR_BUTTON_RELEASE:	return "button-release";
	case HDR_VSCROLL:			return "vscroll";
	case HDR_HSCROLL:			return "hscroll";
	case HDR_MOVE:				return "move";
	default:					return "invalid";
	}
}

static char const *strresp(response_t resp){
	switch(resp){
	case RESP_EINVAL_KEY:	return "invalid key";
	case RESP_EINVAL_CMD:	return "invalid command";
	case RESP_ENOCON:		return "not connected";
	case RESP_OK:			return "ok";
	case RESP_MAGIC:		return "magic";
	default:				return "unknown";
	}
}
