#ifndef RENDER_H_
#define RENDER_H_

#include <SDL.h>

void font_render_init(SDL_Surface * dst, console_t console);
void font_render_done();
void font_render_char(console_t console, SDL_Surface * dst, Sint16 x, Sint16 y, char c);
void font_render_string(console_t console, SDL_Surface * dst, Sint16 x, Sint16 y, char * str);
Uint32 font_render_get_background_color(console_t console);
Uint32 font_render_get_foreground_color(console_t console);

#endif /* RENDER_H_ */
