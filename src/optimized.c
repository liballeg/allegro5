/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Optimized memory blitters
 *
 *      By Trent Gamblin
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_float.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_convert.h"
#include <math.h>

static unsigned char four_bit_lookup[16][16];
static unsigned char five_bit_lookup[32][16];
static unsigned char six_bit_lookup[64][16];
static unsigned char _565_lookup[65536*3];
static unsigned char _4444_lookup[65536*3];

static void init_lookups(void)
{
    int i, j;
    
    for (i = 0; i < 16; i++) {
        for (j = 1; j <= 16; j++) {
            four_bit_lookup[i][j-1] = (char)(i*(j/16.0));
        }
    }
    
    for (i = 0; i < 32; i++) {
        for (j = 1; j <= 16; j++) {
            five_bit_lookup[i][j-1] = (char)(i*(j/16.0));
        }
    }
    
    for (i = 0; i < 64; i++) {
        for (j = 1; j <= 16; j++) {
            six_bit_lookup[i][j-1] = (char)(i*(j/16.0));
        }
    }
    
    for (i = 0; i < 65536; i++) {
        _565_lookup[i*3] = (i & 0xF800) >> 11;
        _565_lookup[i*3+1] = (i & 0x07E0) >> 5;
        _565_lookup[i*3+2] = (i & 0x001F);
        _4444_lookup[i*3] = (i & 0xF000) >> 12;
        _4444_lookup[i*3+1] = (i & 0x0F00) >> 8;
        _4444_lookup[i*3+2] = (i & 0x00F0) >> 4;
    }
}

static bool inited = false;


/* if we get here, all we have to do is blit
 * with no alpha and only blend color
 */
void _al_draw_bitmap_region_optimized_rgb_565_to_rgb_565(
                                                         ALLEGRO_COLOR tint,
                                                         ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
                                                         ALLEGRO_BITMAP *dest, int dx, int dy, int flags)
{
    int xinc, yinc;
    int xd, yd;
    int x, y;
    ALLEGRO_COLOR *bc;
    
    if (!inited) {
        init_lookups();
        inited = true;
    }
    
    if (flags != 0) {
        if (flags & ALLEGRO_FLIP_VERTICAL) {
            yinc = -1;
            yd = dy + sh - 1;
        }
        else {
            yinc = 1;
            yd = dy;
        }
        if (flags & ALLEGRO_FLIP_HORIZONTAL) {
            xinc = -1;
            xd = dx + sw - 1;
        }
        else {
            xinc = 1;
            xd = dx;
        }
    }
    else {
        yinc = 1;
        yd = dy;
        xinc = 1;
        xd = dx;
    }
    
    uint16_t pixel;
    bc = &tint;
    
    char *dest_data = ((char *)dest->memory+yd*dest->pitch+xd*2);
    char *src_data = ((char *)src->memory+sy*src->pitch+sx*2);
    int dest_data_inc;
    int src_data_inc;
    int dest_inc = xinc * 2;
    dest_data_inc = (dest->pitch*yinc) - (sw*dest_inc);
    src_data_inc = src->pitch - (sw*2);
    
    int rindex = bc->r*15;
    int gindex = bc->g*15;
    int bindex = bc->b*15;
    int r, g, b;
    
    for (y = 0; y < sh; y++) {
        for (x = 0; x < sw; x++) {
            pixel = *((uint16_t *)src_data);
            unsigned char *ptr = &(_565_lookup[pixel*3]);
            r = five_bit_lookup[*ptr++][rindex];
            g = six_bit_lookup[*ptr++][gindex];
            b = five_bit_lookup[*ptr][bindex];
            pixel = (r << 11) | (g << 5) | b;
            *((uint16_t *)dest_data) = pixel;
            src_data += 2;
            dest_data += dest_inc;
        }
        src_data += src_data_inc;
        dest_data += dest_data_inc;
    }
}

void _al_draw_bitmap_region_optimized_rgba_4444_to_rgb_565(
                                                           ALLEGRO_COLOR tint,
                                                           ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
                                                           ALLEGRO_BITMAP *dest, int dx, int dy, int flags)
{
    int xinc, yinc;
    int xd, yd;
    int x, y;
    ALLEGRO_COLOR *bc;
    static ALLEGRO_COLOR white = { 1, 1, 1, 1 };
    
    if (!inited) {
        init_lookups();
        inited = true;
    }
    
    if (flags != 0) {
        if (flags & ALLEGRO_FLIP_VERTICAL) {
            yinc = -1;
            yd = dy + sh - 1;
        }
        else {
            yinc = 1;
            yd = dy;
        }
        if (flags & ALLEGRO_FLIP_HORIZONTAL) {
            xinc = -1;
            xd = dx + sw - 1;
        }
        else {
            xinc = 1;
            xd = dx;
        }
    }
    else {
        yinc = 1;
        yd = dy;
        xinc = 1;
        xd = dx;
    }
    
    bc = &tint;
    uint16_t pixel;
    
    char *dest_data = ((char *)dest->memory+yd*dest->pitch+xd*2);
    char *src_data = ((char *)src->memory+sy*src->pitch+sx*2);
    int dest_data_inc;
    int src_data_inc;
    int dest_inc = xinc * 2;
    dest_data_inc = (dest->pitch*yinc) - (sw*dest_inc);
    src_data_inc = src->pitch - (sw*2);
    
    if ((src->flags & ALLEGRO_ALPHA_TEST) && 
        !memcmp(bc, &white, sizeof(ALLEGRO_COLOR))) {
        for (y = 0; y < sh; y++) {
            for (x = 0; x < sw; x++) {
                pixel = *((uint16_t *)src_data);
                if (pixel & 0xF) {
                    *((uint16_t *)dest_data) = ALLEGRO_CONVERT_RGBA_4444_TO_RGB_565(pixel);
                }
                src_data += 2;
                dest_data += dest_inc;
            }
            src_data += src_data_inc;
            dest_data += dest_data_inc;
        }
    }
    else {
        int rindex = bc->r*15;
        int gindex = bc->g*15;
        int bindex = bc->b*15;
        int r, g, b;
        for (y = 0; y < sh; y++) {
            for (x = 0; x < sw; x++) {
                pixel = *((uint16_t *)src_data);
                if (pixel & 0xF) {
                    float alpha = ((pixel & 0xF) / 15.0f) * bc->a;
                    float inv_alpha = 1 - alpha;
                    int alpha_index = alpha*15;
                    int inv_alpha_index = inv_alpha*15;
                    pixel = ALLEGRO_CONVERT_RGBA_4444_TO_RGB_565(pixel);
                    // Below is fast but doesn't look good
                    /*
                     pixel = (pixel & 0xF000) |
                     ((pixel & 0x0F00) >> 1) |
                     ((pixel & 0x00F0) >> 3) | 0x861;
                     */
                    unsigned char *ptr = &(_565_lookup[pixel*3]);
                    r = five_bit_lookup[*ptr++][rindex];
                    g = six_bit_lookup[*ptr++][gindex];
                    b = five_bit_lookup[*ptr][bindex];
                    r = five_bit_lookup[r][alpha_index];
                    g = six_bit_lookup[g][alpha_index];
                    b = five_bit_lookup[b][alpha_index];
                    uint16_t dpix = *((uint16_t *)dest_data);
                    ptr = &(_565_lookup[dpix*3]);
                    int r2 = five_bit_lookup[*ptr++][inv_alpha_index];
                    int g2 = six_bit_lookup[*ptr++][inv_alpha_index];
                    int b2 = five_bit_lookup[*ptr][inv_alpha_index];
                    r = _ALLEGRO_MIN(0x1F, r+r2);
                    g = _ALLEGRO_MIN(0x3F, g+g2);
                    b = _ALLEGRO_MIN(0x1F, b+b2);
                    pixel = (r << 11) | (g << 5) | b;
                    *((uint16_t *)dest_data) = pixel;
                }
                src_data += 2;
                dest_data += dest_inc;
            }
            src_data += src_data_inc;
            dest_data += dest_data_inc;
        }
    }
}


void _al_draw_bitmap_region_optimized_rgba_4444_to_rgba_4444(
                                                             ALLEGRO_COLOR tint,
                                                             ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
                                                             ALLEGRO_BITMAP *dest, int dx, int dy, int flags)
{
    int xinc, yinc;
    int xd, yd;
    int x, y;
    ALLEGRO_COLOR *bc;
    
    if (!inited) {
        init_lookups();
        inited = true;
    }
    
    if (flags != 0) {
        if (flags & ALLEGRO_FLIP_VERTICAL) {
            yinc = -1;
            yd = dy + sh - 1;
        }
        else {
            yinc = 1;
            yd = dy;
        }
        if (flags & ALLEGRO_FLIP_HORIZONTAL) {
            xinc = -1;
            xd = dx + sw - 1;
        }
        else {
            xinc = 1;
            xd = dx;
        }
    }
    else {
        yinc = 1;
        yd = dy;
        xinc = 1;
        xd = dx;
    }
    
    bc = &tint;
    int rindex = bc->r*15;
    int gindex = bc->g*15;
    int bindex = bc->b*15;
    int r, g, b;
    uint16_t pixel;
    
    char *dest_data = ((char *)dest->memory+yd*dest->pitch+xd*2);
    char *src_data = ((char *)src->memory+sy*src->pitch+sx*2);
    int dest_data_inc;
    int src_data_inc;
    int dest_inc = xinc * 2;
    dest_data_inc = (dest->pitch*yinc) - (sw*dest_inc);
    src_data_inc = src->pitch - (sw*2);
    
    if (src->flags & ALLEGRO_ALPHA_TEST) {
        if (bc->r == 1 && bc->g == 1 && bc->b == 1 && bc->a == 1) {
            for (y = 0; y < sh; y++) {
                for (x = 0; x < sw; x++) {
                    pixel = *((uint16_t *)src_data);
                    if (pixel & 0xF) {
                        *((uint16_t *)dest_data) = pixel;
                    }
                    src_data += 2;
                    dest_data += dest_inc;
                }
                src_data += src_data_inc;
                dest_data += dest_data_inc;
            }
        }
        else {
            for (y = 0; y < sh; y++) {
                for (x = 0; x < sw; x++) {
                    pixel = *((uint16_t *)src_data);
                    if (pixel & 0xF) {
                        unsigned char *ptr = &(_4444_lookup[pixel*3]);
                        r = four_bit_lookup[*ptr++][rindex];
                        g = four_bit_lookup[*ptr++][gindex];
                        b = four_bit_lookup[*ptr][bindex];
                        pixel = (r << 12) | (g << 8) | (b << 4) | 0xF;
                        *((uint16_t *)dest_data) = pixel;
                    }
                    src_data += 2;
                    dest_data += dest_inc;
                }
                src_data += src_data_inc;
                dest_data += dest_data_inc;
            }
        }
    }
    else {
        for (y = 0; y < sh; y++) {
            for (x = 0; x < sw; x++) {
                pixel = *((uint16_t *)src_data);
                if (pixel & 0xF) {
                    float alpha = ((pixel & 0xF) / 15.0f) * bc->a;
                    float inv_alpha = 1 - alpha;
                    int alpha_index = alpha*15;
                    int inv_alpha_index = inv_alpha*15;
                    unsigned char *ptr = &(_4444_lookup[pixel*3]);
                    r = four_bit_lookup[*ptr++][alpha_index];
                    g = four_bit_lookup[*ptr++][alpha_index];
                    b = four_bit_lookup[*ptr][alpha_index];
                    uint16_t dpix = *((uint16_t *)dest_data);
                    ptr = &(_4444_lookup[dpix*3]);
                    int r2 = four_bit_lookup[*ptr++][inv_alpha_index];
                    int g2 = four_bit_lookup[*ptr++][inv_alpha_index];
                    int b2 = four_bit_lookup[*ptr][inv_alpha_index];
                    r = four_bit_lookup[(r + r2) & 0xF][rindex];
                    g = four_bit_lookup[(g + g2) & 0xF][gindex];
                    b = four_bit_lookup[(b + b2) & 0xF][bindex];
                    pixel = (r << 12) | (g << 8) | (b << 4) | 0xF;
                    *((uint16_t *)dest_data) = pixel;
                }
                src_data += 2;
                dest_data += dest_inc;
            }
            src_data += src_data_inc;
            dest_data += dest_data_inc;
        }
    }
}

/* vim: set ts=8 sts=3 sw=3 et: */
