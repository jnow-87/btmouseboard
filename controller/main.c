#include <string.h>
#include <controller/events.h>
#include <controller/log.h>
#include <controller/opts.h>
#include <controller/render.h>
#include <controller/uart.h>
#include <controller/xlib.h>


/* global functions */
int main(int argc, char **argv){
	int r;
	uart_t *uart;
	xlib_obj_t *xobj;
	xevent_t ev;


	r = opts_parse(argc, argv);

	if(r != 0)
		return r;

	uart = uart_init();

	if(uart == 0x0)
		goto err_0;

	xobj = xlib_init("btmouseboard");

	if(xobj == 0x0)
		goto err_1;

	// after initialising the log, log messages
	// are shown in the window, instead of stdout
	log_init(opts.debug);

	while(xlib_event(xobj, &ev) == 0){
		if(event_handle(&ev, xobj, uart) > 0)
			break;

		render(xobj, uart);
	}

	xlib_destroy(xobj);
	uart_destroy(uart);

	return 0;


err_1:
	uart_destroy(uart);

err_0:
	return 1;
}
