#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <backend/x11/log.h>
#include <backend/x11/opts.h>
#include <backend/x11/protocol.h>
#include <shared/errlog.h>
#include <shared/socket.h>
#include <shared/xlib.h>


/* local/static prototypes */
static void handle_client(socket_t *client, xlib_obj_t *xobj);


/* global functions */
int main(int argc, char **argv){
	socket_t server,
			 client;
	xlib_obj_t *xobj;


	if(opts_parse(argc, argv) != 0)
		goto_err(err_0, "parsing command line options");

	xobj = xlib_init();

	if(xobj == 0x0)
		goto_err(err_0, "creating x11 connection");

	if(sock_init(&server, 0x0, opts.port) != 0)
		goto_err(err_1, "creating socket");

	if(sock_bind(&server, 1) != 0)
		goto_err(err_2, "binding socket");

	if(!opts.foreground && daemon(0, 0) != 0)
		goto_err(err_2, "daemonising");

	while(1){
		if(sock_accept(&server, &client) != 0)
			continue;

		INFO("client connected %s", inet_ntoa(client.addr.sin_addr));
		handle_client(&client, xobj);
	}

	sock_close(&server);
	xlib_destroy(xobj);

	return 0;


err_2:
	sock_close(&server);

err_1:
	xlib_destroy(xobj);

err_0:
	ERROR("initialisation%s%s", (errno ? ": " : ""), (errno ? strerror(errno) : ""));

	return 1;
}


/* local functions */
static void handle_client(socket_t *client, xlib_obj_t *xobj){
	int r = 0;
	dgram_t dgram;
	response_t resp;


	while(1){
		if(sock_recv(client, &dgram, sizeof(dgram)) != 0)
			break;

		switch(dgram.cmd){
		case CMD_KEY:
			DEBUG("key: %s %s", XKeysymToString(dgram.sym), dgram.pressed ? "press" : "release");
			r = xlib_key(xobj, dgram.sym, dgram.pressed);
			break;

		case CMD_BUTTON:
			DEBUG("button: %d %s", dgram.button, dgram.pressed ? "press" : "release");
			r = xlib_cursor_click(xobj, dgram.button, dgram.pressed);
			break;

		case CMD_MOVE:
			DEBUG("move: %d %d", dgram.dx, dgram.dy);
			r = xlib_cursor_move(xobj, dgram.dx, dgram.dy);
			break;

		default:
			ERROR("invalid cmd %u", dgram.cmd);
			resp = RESP_ECMD;
			break;
		}

		resp = (r == 0) ? RESP_OK : RESP_EXLIB;
		sock_send(client, &resp, sizeof(resp));
		xlib_sync(xobj);
	}

	sock_close(client);
	INFO("client connection closed %s", inet_ntoa(client->addr.sin_addr));
}
