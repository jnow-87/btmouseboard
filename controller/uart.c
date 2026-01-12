#include <config/config.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <controller/log.h>
#include <controller/render.h>
#include <controller/uart.h>
#include <protocol.h>


/* macros */
#define _BRATE(x)				B##x
#define TERMIOS_BRATE(baud)		_BRATE(baud)


/* local/static prototypes */
static int hscroll(uart_t *uart, uint8_t button);
static int vscroll(uart_t *uart, uint8_t button);

static int send_cmd(uart_t *uart, hdr_t hdr, uint8_t *data, size_t ndata);
static int trywrite(uart_t *uart, uint8_t *data, size_t n);

static void reinit(uart_t *uart);
static void discover(uart_t *uart, char const *fmt);
static int configure(int fd);

static char const *strcmd(hdr_t hdr);
static char const *strresp(response_t resp);


/* global functions */
uart_t *uart_init(void){
	uart_t *uart;


	uart = malloc(sizeof(uart_t));

	if(uart == 0x0){
		ERROR("allocating uart");

		return 0x0;
	}

	uart->fd = -1;
	reinit(uart);

	return uart;
}

void uart_destroy(uart_t *uart){
	if(uart->fd >= 0){
		uart_stop(uart);
		close(uart->fd);
	}

	free(uart);
}

int uart_stop(uart_t *uart){
	return send_cmd(uart, HDR_CLOSE, 0x0, 0);
}

int uart_key(uart_t *uart, uint8_t key, bool press){
	return send_cmd(uart, press ? HDR_KEY_PRESS : HDR_KEY_RELEASE, &key, 1);
}

int uart_button(uart_t *uart, uint8_t button, bool press){
	if(button == 4 || button == 5)
		return vscroll(uart, button);

	if(button == 6 || button == 7)
		return hscroll(uart, button);

	return send_cmd(uart, press ? HDR_BUTTON_PRESS : HDR_BUTTON_RELEASE, &button, 1);
}

int uart_move(uart_t *uart, int8_t dx, int8_t dy){
	return send_cmd(uart, HDR_MOVE, (uint8_t []){dx, dy}, 2);
}


/* local functions */
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
