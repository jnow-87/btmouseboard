#include <config/config.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <backend/backend.h>
#include <backend/x11/protocol.h>
#include <controller/log.h>
#include <controller/render.h>
#include <shared/socket.h>


/* macros */
#define DGRAM_KEY(_sym, _pressed)		(&(dgram_t){ .cmd = CMD_KEY, .sym = _sym, .pressed = _pressed })
#define DGRAM_BUTTON(_button, _pressed)	(&(dgram_t){ .cmd = CMD_BUTTON, .button = _button, .pressed = _pressed })
#define DGRAM_MOVE(_dx, _dy)			(&(dgram_t){ .cmd = CMD_MOVE, .dx = _dx, .dy = _dy })


/* local/static prototypes */
static void be_destroy(struct backend_t *be);
static int be_stop(struct backend_t *be);
static int be_key(struct backend_t *be, KeySym sym, bool press);
static int be_button(struct backend_t *be, uint8_t button, bool press);
static int be_move(struct backend_t *be, int8_t dx, int8_t dy);
static unsigned int be_render_status(struct backend_t *be, xlib_win_t *win, unsigned int x, unsigned int y);

static int send_dgram(socket_t *sock, dgram_t *dgram);
static char const *strcmd(cmd_t cmd);
static char const *strresp(response_t resp);


/* global functions */
backend_t *backend_create_x11(char const *host, unsigned int port){
	backend_t *be;
	socket_t *sock;


	be = malloc(sizeof(backend_t));

	if(be == 0x0)
		goto err_0;

	sock = malloc(sizeof(socket_t));

	if(sock == 0x0)
		goto err_1;

	if(sock_init(sock, host, port) != 0)
		goto err_2;

	be->data = sock;
	be->destroy = be_destroy;
	be->stop = be_stop;
	be->key = be_key;
	be->button = be_button;
	be->move = be_move;
	be->render_status = be_render_status;

	// ignore SIGPIPE to avoid a lost connection to terminate the program
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		goto err_3;

	return be;


err_3:
	sock_close(sock);

err_2:
	free(sock);

err_1:
	free(be);

err_0:
	ERROR("allocating x11 backend: %s", strerror(errno));

	return 0x0;
}


/* local functions */
static void be_destroy(struct backend_t *be){
	socket_t *sock = (socket_t*)be->data;


	close(sock->fd);
	free(sock);
	free(be);
}

static int be_stop(struct backend_t *be){
	sock_close(be->data);

	return 0;
}

static int be_key(struct backend_t *be, KeySym sym, bool press){
	return send_dgram((socket_t*)be->data, DGRAM_KEY(sym, press));
}

static int be_button(struct backend_t *be, uint8_t button, bool press){
	return send_dgram((socket_t*)be->data, DGRAM_BUTTON(button, press));
}

static int be_move(struct backend_t *be, int8_t dx, int8_t dy){
	return send_dgram((socket_t*)be->data, DGRAM_MOVE(dx, dy));
}

static unsigned int be_render_status(struct backend_t *be, xlib_win_t *win, unsigned int x, unsigned int y){
	unsigned int dx = 0;
	socket_t *sock = (socket_t*)be->data;


	if(sock->fd != -1){
		dx += xlib_cprintf(win, x, y,  COLOR_SUCCESS, " 󰌘  ");
		dx += xlib_cprintf(win, x + dx, y, COLOR_TEXT, inet_ntoa(sock->addr.sin_addr));
	}
	else
		dx += xlib_cprintf(win, x, y, COLOR_TEXT, " 󰌙  ");

	return dx;
}

static int send_dgram(socket_t *sock, dgram_t *dgram){
	int connected = (sock->fd != -1);
	response_t resp;


	if(sock->fd == -1 && sock_connect(sock) != 0)
		goto err;

	if(sock_send(sock, dgram, sizeof(dgram_t)) != 0)
		goto err;

	if(sock_recv(sock, &resp, sizeof(resp)) != 0)
		goto err;

	DEBUG("send dgram %s: %s", strcmd(dgram->cmd), strresp(resp));

	if(!connected)
		render_mark();

	return 0;


err:
	ERROR("lost connection to host");
	sock_close(sock);

	if(connected)
		render_mark();

	return -1;
}

static char const *strcmd(cmd_t cmd){
	switch(cmd){
	case CMD_KEY:		return "key";
	case CMD_BUTTON:	return "button";
	case CMD_MOVE:		return "move";
	default:			return "invalid";
	}
}

static char const *strresp(response_t resp){
	switch(resp){
	case RESP_ECMD:		return "invalid command";
	case RESP_EXLIB:	return "xlib error";
	case RESP_OK:		return "ok";
	default:			return "unknown";
	}
}
