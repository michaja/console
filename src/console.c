#include "console.h"
#include "font.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

struct cell {
    union {
        struct {
            unsigned char character;
            unsigned char attribute;
        } cell;
        unsigned short cell_data;
    };
};

struct console {
    unsigned height;
    unsigned width;

    unsigned view_height;
    unsigned view_width;

    unsigned char_height;
    unsigned char_width;

    unsigned cursor_x;
    unsigned cursor_y;
    unsigned saved_cursor_x;
    unsigned saved_cursor_y;
    unsigned char attribute;

    struct cell * buffer;

    console_mode mode;
    unsigned tab_width;
    unsigned char cursor_state;
    unsigned cursor_blink_rate;
    console_rgb_t palette[16];
    font_id_t font_id;
    console_callback_t callback;
    void * callback_data;
};

#define CURSOR_VISIBLE 1
#define CURSOR_SHOWN 2

/* http://en.wikipedia.org/wiki/ANSI_escape_code*/
static console_rgb_t g_palette[] = {
    /* normal */
    {0x00, 0x00, 0x00},    /* 0  black */
    {0x00, 0x00, 0xAA},    /* 1  blue */
    {0x00, 0xAA, 0x00},    /* 2  green */
    {0x00, 0xAA, 0xAA},    /* 3  cyan */
    {0xAA, 0x00, 0x00},    /* 4  red */
    {0xAA, 0x00, 0xAA},    /* 5  magenta */
    {0xAA, 0x55, 0x00},    /* 6  brown */
    {0xAA, 0xAA, 0xAA},    /* 7  light gray */
    /* bright */
    {0x55, 0x55, 0x55},    /* 8  gray */
    {0x55, 0x55, 0xFF},    /* 9  light blue */
    {0x55, 0xFF, 0x55},    /* 10 light green */
    {0x55, 0xFF, 0xFF},    /* 11 light cyan */
    {0xFF, 0x55, 0x55},    /* 12 light red */
    {0xFF, 0x55, 0xFF},    /* 13 light magenta */
    {0xFF, 0xFF, 0x55},    /* 14 yellow */
    {0xFF, 0xFF, 0xFF},    /* 15 white */
};

static void console_callback(console_t console, console_update_t * p, void * data) {
}

console_t console_alloc(unsigned width, unsigned height, font_id_t font) {
    console_t console = (console_t)calloc(1, sizeof(struct console));
    console->view_width = width;
    console->view_height = height;
    console_set_mode(console, CONSOLE_MODE_RAW);
    console_set_tab_width(console, 4);
    console_set_callback(console, NULL, NULL);
    console_set_palette(console, &g_palette[0]);
    console_set_font(console, font);
    console_set_cursor_blink_rate(console, 200);
    console_show_cursor(console);
    console_clear(console);
    return console;
}

void console_free(console_t console) {
    if(console) {
        console->callback_data = NULL;
        free(console->buffer);
        free(console);
    }
}

unsigned short * console_get_raw_buffer(console_t console) {
    return (unsigned short*)console->buffer;
}

void console_save_cursor_position(console_t console) {
    console->saved_cursor_x = console->cursor_x;
    console->saved_cursor_y = console->cursor_y;
}

void console_restore_cursor_position(console_t console) {
    console->cursor_x = console->saved_cursor_x;
    console->cursor_y = console->saved_cursor_y;
}

void console_set_tab_width(console_t console, unsigned width) {
    console->tab_width = 4;
}

unsigned console_get_tab_width(console_t console) {
    return console->tab_width;
}

bool console_cursor_is_visible(console_t console) {
    return console->cursor_state & CURSOR_VISIBLE;
}

bool console_cursor_is_shown(console_t console) {
    return (console->cursor_state & (CURSOR_VISIBLE | CURSOR_SHOWN)) == (CURSOR_VISIBLE | CURSOR_SHOWN);
}

void console_set_cursor_blink_rate(console_t console, unsigned rate) {
    console->cursor_blink_rate = rate;
}

unsigned console_get_cursor_blink_rate(console_t console) {
    return console->cursor_blink_rate;
}

void console_show_cursor(console_t console) {
    if(console->cursor_state & CURSOR_VISIBLE)
        return;
    console->cursor_state |= CURSOR_VISIBLE;
    console_update_t u;
    u.type = CONSOLE_UPDATE_CURSOR_VISIBILITY;
    u.data.u_cursor.cursor_visible = true;
    u.data.u_cursor.x = console->cursor_x;
    u.data.u_cursor.y = console->cursor_y;
    console->callback(console, &u, console->callback_data);
}

void console_hide_cursor(console_t console) {
    if(!(console->cursor_state & CURSOR_VISIBLE))
        return;
    console->cursor_state &= ~(CURSOR_VISIBLE | CURSOR_SHOWN);
    console_update_t u;
    u.type = CONSOLE_UPDATE_CURSOR_VISIBILITY;
    u.data.u_cursor.cursor_visible = false;
    u.data.u_cursor.x = console->cursor_x;
    u.data.u_cursor.y = console->cursor_y;
    console->callback(console, &u, console->callback_data);
}

void console_blink_cursor(console_t console) {
    if(!console_cursor_is_visible(console))
        return; /* cursor is not visible */
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t milliseconds = (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000;
    bool shown = (milliseconds % (console->cursor_blink_rate * 2)) < console->cursor_blink_rate ? true : false;
    bool was_shown = (((console->cursor_state & CURSOR_SHOWN) >> 1) ? true : false);
    if(shown != was_shown) {
        console->cursor_state ^= CURSOR_SHOWN;
        console_update_t u;
        u.type = CONSOLE_UPDATE_CURSOR_VISIBILITY;
        u.data.u_cursor.cursor_visible = console_cursor_is_shown(console);
        u.data.u_cursor.x = console->cursor_x;
        u.data.u_cursor.y = console->cursor_y;
        console->callback(console, &u, console->callback_data);
    }
}

void console_set_callback(console_t console, console_callback_t callback, void * data) {
    console->callback = callback == 0 ? console_callback : callback;
    console->callback_data = data;
}

void console_set_palette(console_t console, console_rgb_t const * palette) {
    memcpy(console->palette, palette, sizeof(console_rgb_t) * 16);
    console_update_t u;
    u.type = CONSOLE_UPDATE_PALETTE;
    u.data.u_palette.palette = console->palette;
    console->callback(console, &u, console->callback_data);
}

void console_get_palette(console_t console, console_rgb_t * palette) {
    memcpy(palette, console->palette, sizeof(console_rgb_t) * 16);
}

void console_set_font(console_t console, font_id_t font) {
    if(console->font_id == font)
        return;

    console->font_id = font;

    console->char_height = console_fonts[font].char_height;
    console->char_width = console_fonts[font].char_width;

    console->width = console->view_width / console->char_width;
    console->height = console->view_height / console->char_height;

    size_t num_cells = console->width * console->height;
    console->buffer = realloc(console->buffer, num_cells * sizeof(struct cell));

    console_update_t u;
    u.type = CONSOLE_UPDATE_FONT;
    u.data.u_font.char_width = console->char_width;
    u.data.u_font.char_height = console->char_height;
    u.data.u_font.font_bitmap = console_fonts[font].font_bitmap;
    console->callback(console, &u, console->callback_data);
}

font_id_t console_get_font(console_t console) {
    return console->font_id;
}

static void console_update_rows(console_t console, unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
    console_update_t u;
    u.type = CONSOLE_UPDATE_ROWS;
    u.data.u_rows.x1 = x1;
    u.data.u_rows.y1 = y1;
    u.data.u_rows.x2 = x2;
    u.data.u_rows.y2 = y2;
    console->callback(console, &u, console->callback_data);
}

void console_clear(console_t console) {
    console_cursor_goto_xy(console, 0, 0);
    console->attribute = 0xf;
    unsigned x = console->width * console->height;
    struct cell * buffer = console->buffer;
    for(;x > 0; --x, ++buffer)
        buffer->cell_data = 0x7;
    console_update_rows(console, 0, 0, console->width, console->height);
}

static void console_cursor_advance(console_t console) {
    unsigned x = console->cursor_x;
    unsigned y = console->cursor_y;
    if(++x >= console->width) {
        x = 0;
        if(++y >= console->height) {
            y = console->height-1;
            console_scroll_lines(console, 1);
        }
    }
    console_cursor_goto_xy(console, x, y);
}

void console_print_char(console_t console, unsigned char c) {
    if(c == '\n') {
        unsigned x = 0;
        unsigned y = console->cursor_y;
        if(++y >= console->height) {
            y = console->height - 1;
            console_scroll_lines(console, 1);
        }
        console_cursor_goto_xy(console, x, y);
        return;
    }
    if(c == '\t') {
        unsigned i;
        for(i = 0; i < console->tab_width; i++) {
            console_print_char(console, ' ');
        }
        return;
    }

    if(console->mode == CONSOLE_MODE_RAW) {
        size_t offset = console->cursor_y * console->width + console->cursor_x;
        unsigned char old_c = console->buffer[offset].cell.character;
        unsigned char old_a = console->buffer[offset].cell.attribute;
        console->buffer[offset].cell.character = c;
        console->buffer[offset].cell.attribute = console->attribute;

        if(old_c != c || old_a != console->attribute) {
            console_update_t u;
            u.type = CONSOLE_UPDATE_CHAR;
            u.data.u_char.x = console->cursor_x;
            u.data.u_char.y = console->cursor_y;
            u.data.u_char.c = c;
            u.data.u_char.a = console->attribute;
            console->callback(console, &u, console->callback_data);
        }

        console_cursor_advance(console);
    }
}

void console_set_attribute(console_t console, unsigned char attr) {
    console->attribute = attr;
}

void console_set_character_and_attribute_at(console_t console, unsigned x, unsigned y, unsigned char c, unsigned char attr) {
    if(x >= console->width || y >= console->height)
        return;
    size_t offset = y * console->width + x;
    unsigned char old_c = console->buffer[offset].cell.character;
    unsigned char old_a = console->buffer[offset].cell.attribute;
    console->buffer[offset].cell.character = c;
    console->buffer[offset].cell.attribute = attr;

    if(old_c != c || old_a != attr) {
        console_update_t u;
        u.type = CONSOLE_UPDATE_CHAR;
        u.data.u_char.x = x;
        u.data.u_char.y = y;
        u.data.u_char.c = c;
        u.data.u_char.a = attr;
        console->callback(console, &u, console->callback_data);
    }
}

void console_set_character_and_attribute_at_offset(console_t console, unsigned offset, unsigned char c, unsigned char attr) {
    if(offset >= console->width * console->height)
        return;
    unsigned char old_c = console->buffer[offset].cell.character;
    unsigned char old_a = console->buffer[offset].cell.attribute;
    console->buffer[offset].cell.character = c;
    console->buffer[offset].cell.attribute = attr;

    if(old_c != c || old_a != attr) {
        console_update_t u;
        u.type = CONSOLE_UPDATE_CHAR;
        u.data.u_char.x = offset % console->width;
        u.data.u_char.y = offset / console->width;
        u.data.u_char.c = c;
        u.data.u_char.a = attr;
        console->callback(console, &u, console->callback_data);
    }
}

void console_cursor_goto_xy(console_t console, unsigned x, unsigned y) {
    if(x >= console->width)
        x = console->width - 1;
    if(y >= console->height)
        y = console->height - 1;
    if(x!=console->cursor_x || y!=console->cursor_y) {
        console_update_t u;
        u.type = CONSOLE_UPDATE_CURSOR_POSITION;
        u.data.u_cursor.cursor_visible = true;
        u.data.u_cursor.x = console->cursor_x;
        u.data.u_cursor.y = console->cursor_y;

        console->cursor_x = x;
        console->cursor_y = y;

        console->callback(console, &u, console->callback_data);
    }
}

void console_scroll_lines(console_t console, unsigned n) {
    if(n == 0)
        return;
    unsigned w = console->width;
    unsigned h = console->height;

    console_update_t u;
    u.type = CONSOLE_UPDATE_CURSOR_VISIBILITY;
    u.data.u_cursor.cursor_visible = false;
    u.data.u_cursor.x = console->cursor_x;
    u.data.u_cursor.y = console->cursor_y;
    console->callback(console, &u, console->callback_data);

    u.type = CONSOLE_UPDATE_SCROLL;
    u.data.u_scroll.y1 = 0;
    u.data.u_scroll.y2 = min(h, n);
    u.data.u_scroll.n = h - min(h, n);
    console->callback(console, &u, console->callback_data);

    unsigned offset = n * w;
    if(n < console->height) {
        size_t bytes = (console->height - n) * w * sizeof(unsigned short);
        memmove(console->buffer, console->buffer + offset, bytes);
    }
    if(n < h) {
        offset = (h - n) * w;
        n = n * w;
        struct cell * buffer = console->buffer + offset;
        for(; n > 0; --n, ++buffer) {
            buffer->cell.character = 0;
            buffer->cell.attribute = console->attribute;
        }
    } else {
        n = h * w;
        struct cell * buffer = console->buffer;
        for(; n > 0; --n, ++buffer) {
            buffer->cell.character = 0;
            buffer->cell.attribute = console->attribute;
        }
    }

    u.type = CONSOLE_UPDATE_CURSOR_VISIBILITY;
    u.data.u_cursor.cursor_visible = console_cursor_is_shown(console);
    u.data.u_cursor.x = console->cursor_x;
    u.data.u_cursor.y = console->cursor_y;
    console->callback(console, &u, console->callback_data);
}

unsigned console_get_width(console_t console) {
    return console->width;
}

unsigned console_get_height(console_t console) {
    return console->height;
}

unsigned console_get_char_width(console_t console) {
    return console->char_width;
}

unsigned console_get_char_height(console_t console) {
    return console->char_height;
}

unsigned console_get_cursor_x(console_t console) {
    return console->cursor_x;
}

unsigned console_get_cursor_y(console_t console) {
    return console->cursor_y;
}

unsigned char console_get_character_at(console_t console, unsigned x, unsigned y) {
    if(x >= console->width || y >= console->height)
        return 0;
    return console->buffer[y * console->width + x].cell.character;
}

unsigned char console_get_character_at_offset(console_t console, unsigned offset) {
    if(offset >= console->width * console->height)
        return 0;
    return console->buffer[offset].cell.character;
}

unsigned char console_get_attribute_at(console_t console, unsigned x, unsigned y) {
    if(x >= console->width || y >= console->height)
        return 0;
    return console->buffer[y * console->width + x].cell.attribute;
}

unsigned char console_get_attribute_at_offset(console_t console, unsigned offset) {
    if(offset >= console->width * console->height)
        return 0;
    return console->buffer[offset].cell.attribute;
}

unsigned char console_get_background_color(console_t console) {
    return (console->attribute & 0xf0) >> 4;
}

unsigned char console_get_foreground_color(console_t console) {
    return console->attribute & 0xf;
}

unsigned char * console_get_char_bitmap(console_t console, unsigned char c) {
    unsigned bytes_per_row = (console->char_width + 7) / 8;
    unsigned bytes_per_char = bytes_per_row * console->char_height;
    unsigned offset = c * bytes_per_char;
    return &console_fonts[console->font_id].font_bitmap[offset];
}

void console_set_mode(console_t console, console_mode mode) {
    console->mode = mode;
}

console_mode console_get_mode(console_t console) {
    return console->mode;
}

void console_refresh(console_t console) {
    console_update_t u;
    u.type = CONSOLE_UPDATE_REFRESH;
    console->callback(console, &u, console->callback_data);
}

