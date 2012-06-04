#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <sys/types.h>

struct console_rgb {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

struct console_cell_attribute {
    struct console_rgb foreground;
    struct console_rgb background;
};

struct console_cell {
    char character;
    struct console_cell_attribute attribute;
};

struct console;
typedef struct console * console_t;

console_t console_alloc(unsigned width, unsigned height);
void console_free(console_t console);
void console_clear(console_t console);
void console_print_char(console_t console, int c);
void console_print_string(console_t console, const char * str);
void console_set_foreground_attr_rgb(console_t console, unsigned char r, unsigned char g, unsigned char b);
void console_set_background_attr_rgb(console_t console, unsigned char r, unsigned char g, unsigned char b);
void console_goto_xy(console_t console, unsigned x, unsigned y);
void console_scroll_lines(console_t console, unsigned y);
unsigned console_get_width(console_t console);
unsigned console_get_height(console_t console);
unsigned console_get_char_width(console_t console);
unsigned console_get_char_height(console_t console);
unsigned console_get_x(console_t console);
unsigned console_get_y(console_t console);
int console_get_char_at(console_t console, unsigned x, unsigned y);
void console_get_string_at(console_t console, unsigned x, unsigned y, char * buffer, size_t num_bytes);

#endif /* CONSOLE_H_ */
