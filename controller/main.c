#include <string.h>
#include <backend/backend.h>
#include <controller/events.h>
#include <controller/log.h>
#include <controller/opts.h>
#include <controller/render.h>
#include <shared/errlog.h>
#include <shared/xlib.h>


/* global functions */
int main(int argc, char **argv){
	int r;
	backend_t *be;
	xlib_obj_t *xobj;
	xlib_win_t *win;
	xevent_t ev;


	r = opts_parse(argc, argv);

	if(r != 0)
		return r;

	be = backend_create_uart();

	if(be == 0x0)
		goto_err(err_0, "creating backend");

	xobj = xlib_init();

	if(xobj == 0x0)
		goto_err(err_1, "creating x11 connection");

	win = xlib_win_create(xobj, "mb");

	if(win == 0x0)
		goto_err(err_2, "creating x11 window");

	// after initialising the log, log messages
	// are shown in the window, instead of stdout
	log_init(opts.debug);

	while(xlib_event(xobj, &ev) == 0){
		if(event_handle(&ev, win, be) > 0)
			break;

		render(win, be);
	}

	xlib_win_destroy(win);
	xlib_destroy(xobj);
	be->destroy(be);

	return 0;


err_2:
	xlib_destroy(xobj);

err_1:
	be->destroy(be);

err_0:
	return 1;
}
