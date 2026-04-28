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

/* global functions */
void render(xlib_win_t *win, backend_t *be){
	unsigned int x = 0,
				 y = 0;
	size_t log_lines;
	log_entry_t *entry;


	if(!render_requested)
		return;

	xlib_scene_begin(win);

	log_lines = (win->height - 1.5 * win->gfx->font_height) / win->gfx->font_height;

	for(size_t i=0; (entry=log_cycle(log_lines))!=0x0; i++){
		x = 0;
		x += xlib_cprintf(win, x, y, COLOR_TEXT, entry->time);
		x += xlib_cprintf(win, x, y, COLOR_TEXT, ":");
		x += xlib_cprintf(win, x, y, log_level_color[entry->level], log_strlevel(entry->level));
		x += xlib_cprintf(win, x, y, COLOR_TEXT, ":");
		x += xlib_cprintf(win, x, y, COLOR_TEXT, entry->text);
		y += win->gfx->font_height;
	}

	x = 0;
	y = win->height - 1.5 * win->gfx->font_height;

	xlib_rect(win, 0, y, win->width, win->gfx->font_height * 1.5, COLOR_STATUSLINE, true);
	x += be->render_status(be, win, x, y);

	xlib_scene_end(win);

	render_requested = false;
}

void render_mark(void){
	render_requested = true;
}
