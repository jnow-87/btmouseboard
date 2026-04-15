#include <config/config.h>
#include <stdbool.h>
#include <controller/log.h>
#include <controller/render.h>
#include <shared/xlib.h>


/* static variables */
static bool render_requested = false;

static color_t log_level_color[] = {
	[LOG_INFO] = COLOR_INFO,
	[LOG_ERROR] = COLOR_ERROR,
	[LOG_DEBUG] = COLOR_TEXT,
};

static char const *log_level_prefix[] = {
	[LOG_INFO] = ":INF:",
	[LOG_ERROR] = ":ERR:",
	[LOG_DEBUG] = ":DBG:",
};


/* global functions */
void render(xlib_obj_t *xobj, backend_t *be){
	unsigned int x = 0,
				 y = 0;
	size_t log_lines;
	log_entry_t *entry;


	if(!render_requested)
		return;

	xlib_scene_begin(xobj);

	log_lines = (xobj->win_height - 1.5 * xobj->gfx->font_height) / xobj->gfx->font_height;

	for(size_t i=0; (entry=log_cycle(log_lines))!=0x0; i++){
		x = 0;
		x += xlib_cprintf(xobj, x, y, COLOR_TEXT, entry->time);
		x += xlib_cprintf(xobj, x, y, log_level_color[entry->level], log_level_prefix[entry->level]);
		x += xlib_cprintf(xobj, x, y, COLOR_TEXT, entry->text);
		y += xobj->gfx->font_height;
	}

	x = 0;
	y = xobj->win_height - 1.5 * xobj->gfx->font_height;

	xlib_rect(xobj, 0, y, xobj->win_width, xobj->gfx->font_height * 1.5, COLOR_STATUSLINE, true);
	x += be->render_status(be, xobj, x, y);

	xlib_scene_end(xobj);

	render_requested = false;
}

void render_mark(void){
	render_requested = true;
}
