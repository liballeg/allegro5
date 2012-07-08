#!/usr/bin/env python
from __future__ import division
import sys, os
from random import *
from math import *
from ctypes import *

# Get path to examples data.
p = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "examples"))

from allegro import *

def abort_example(text):
    sys.stderr.write(text)
    sys.exit(1)

FPS = 60
MAX_SPRITES = 1024

class Sprite:
    def __init__(self):
        self.x, self.y, self.dx, self.dy = 0, 0, 0, 0

text = [
   "Space - toggle use of textures",
   "B - toggle alpha blending",
   "Left/Right - change bitmap size",
   "Up/Down - change bitmap count",
   "F1 - toggle help text"
    ]

class Example:
   sprites = [Sprite() for i in range(MAX_SPRITES)]
   use_memory_bitmaps = False
   blending = False
   display = None
   mysha = None
   bitmap = None
   bitmap_size = 0
   sprite_count = 0
   show_help = True
   font = None
    
   mouse_down = False
   last_x, last_y = 0, 0

   white = None
   half_white = None
   dark = None
   red = None
   
   direct_speed_measure = 1.0

   ftpos = 0
   frame_times = [0.0] * FPS

example = Example()

def add_time():
   example.frame_times[example.ftpos] = al_get_time()
   example.ftpos += 1
   if example.ftpos >= FPS:
      example.ftpos = 0

def get_fps():
    prev = FPS - 1
    min_dt = 1
    max_dt = 1 / 1000000
    av = 0
    for i in range(FPS):
        if i != example.ftpos:
            dt = example.frame_times[i] - example.frame_times[prev]
            if dt < min_dt and dt > 0:
                min_dt = dt
            if dt > max_dt:
                max_dt = dt
            av += dt
        prev = i
    av /= FPS - 1
    average = ceil(1 / av)
    d = 1 / min_dt - 1 / max_dt
    minmax = floor(d / 2)
    return average, minmax

def add_sprite():
    if example.sprite_count < MAX_SPRITES:
        w = al_get_display_width(example.display)
        h = al_get_display_height(example.display)
        i = example.sprite_count
        example.sprite_count += 1
        s = example.sprites[i]
        a = randint(0, 359)
        s.x = randint(0, w - example.bitmap_size - 1)
        s.y = randint(0, h - example.bitmap_size - 1)
        s.dx = cos(a) * FPS * 2
        s.dy = sin(a) * FPS * 2

def add_sprites(n):
    for i in range(n):
        add_sprite()

def remove_sprites(n):
    example.sprite_count -= n
    if example.sprite_count < 0:
        example.sprite_count = 0

def change_size(size):
   if size < 1:
      size = 1
   if size > 1024:
      size = 1024

   if example.bitmap:
      al_destroy_bitmap(example.bitmap)
   al_set_new_bitmap_flags(
      ALLEGRO_MEMORY_BITMAP if example.use_memory_bitmaps else 0)
   example.bitmap = al_create_bitmap(size, size)
   example.bitmap_size = size
   al_set_target_bitmap(example.bitmap)
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO, example.white)
   al_clear_to_color(al_map_rgba_f(0, 0, 0, 0))
   bw = al_get_bitmap_width(example.mysha)
   bh = al_get_bitmap_height(example.mysha)
   al_draw_scaled_bitmap(example.mysha, 0, 0, bw, bh, 0, 0,
      size, size, 0)
   al_set_target_backbuffer(example.display)

def sprite_update(s):
   w = al_get_display_width(example.display)
   h = al_get_display_height(example.display)

   s.x += s.dx / FPS
   s.y += s.dy / FPS

   if s.x < 0:
      s.x = -s.x
      s.dx = -s.dx

   if s.x + example.bitmap_size > w:
      s.x = -s.x + 2 * (w - example.bitmap_size)
      s.dx = -s.dx

   if s.y < 0:
      s.y = -s.y
      s.dy = -s.dy

   if s.y + example.bitmap_size > h:
      s.y = -s.y + 2 * (h - example.bitmap_size)
      s.dy = -s.dy

   if example.bitmap_size > w: s.x = w / 2 - example.bitmap_size / 2
   if example.bitmap_size > h: s.y = h / 2 - example.bitmap_size / 2

def update():
   for i in range(example.sprite_count):
      sprite_update(example.sprites[i])

def redraw():
    w = al_get_display_width(example.display)
    h = al_get_display_height(example.display)
    fh = al_get_font_line_height(example.font)
    info = ["textures", "memory buffers"]
    binfo = ["alpha", "additive", "tinted", "solid"]
    tint = example.white

    if example.blending == 0:
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA)
        tint = example.half_white
    elif example.blending == 1:
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE)
        tint = example.dark
    elif example.blending == 2:
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO)
        tint = example.red
    elif example.blending == 3:
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO, example.white)

    for i in range(example.sprite_count):
        s = example.sprites[i]
        al_draw_tinted_bitmap(example.bitmap, tint, s.x, s.y, 0)

    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA)
    if example.show_help:
        for i in range(5):
            al_draw_text(example.font, example.white, 0, h - 5 * fh + i * fh, 0, text[i])

    al_draw_textf(example.font, example.white, 0, 0, 0, "count: %d",
        example.sprite_count)
    al_draw_textf(example.font, example.white, 0, fh, 0, "size: %d",
        example.bitmap_size)
    al_draw_textf(example.font, example.white, 0, fh * 2, 0, "%s",
        info[example.use_memory_bitmaps].encode("utf8"))
    al_draw_textf(example.font, example.white, 0, fh * 3, 0, "%s",
        binfo[example.blending].encode("utf8"))

    f1, f2 = get_fps()
    al_draw_textf(example.font, example.white, w, 0, ALLEGRO_ALIGN_RIGHT, "%s",
        ("FPS: %4d +- %-4d" % (f1, f2)).encode("utf8"))
    al_draw_textf(example.font, example.white, w, fh, ALLEGRO_ALIGN_RIGHT, "%s",
        ("%4d / sec" % int(1.0 / example.direct_speed_measure)).encode("utf8"))

def main():
    w, h = 640, 480
    done = False
    need_redraw = True
    example.show_help = True

    if not al_install_system(ALLEGRO_VERSION_INT, None):
        abort_example("Failed to init Allegro.\n")
        sys.exit(1)

    if not al_init_image_addon():
        abort_example("Failed to init IIO addon.\n")
        sys.exit(1)

    al_init_font_addon()

    al_get_num_video_adapters()
   
    info = ALLEGRO_MONITOR_INFO()
    al_get_monitor_info(0, byref(info))
    if info.x2 - info.x1 < w:
        w = info.x2 - info.x1
    if info.y2 - info.y1 < h:
        h = info.y2 - info.y1
    example.display = al_create_display(w, h)

    if not example.display:
        abort_example("Error creating display.\n")

    if not al_install_keyboard():
        abort_example("Error installing keyboard.\n")
    
    if not al_install_mouse():
        abort_example("Error installing mouse.\n")

    example.font = al_load_font(p + "/data/fixed_font.tga", 0, 0)
    if not example.font:
        abort_example("Error loading data/fixed_font.tga\n")

    example.mysha = al_load_bitmap(p + "/data/mysha256x256.png")
    if not example.mysha:
        abort_example("Error loading data/mysha256x256.png\n")

    example.white = al_map_rgb_f(1, 1, 1)
    example.half_white = al_map_rgba_f(1, 1, 1, 0.5)
    example.dark = al_map_rgb(15, 15, 15)
    example.red = al_map_rgb_f(1, 0.2, 0.1)
    change_size(256)
    add_sprite()
    add_sprite()

    timer = al_create_timer(1.0 / FPS)

    queue = al_create_event_queue()
    al_register_event_source(queue, al_get_keyboard_event_source())
    al_register_event_source(queue, al_get_mouse_event_source())
    al_register_event_source(queue, al_get_timer_event_source(timer))
    al_register_event_source(queue, al_get_display_event_source(example.display))

    al_start_timer(timer)

    while not done:
        event = ALLEGRO_EVENT()

        if need_redraw and al_is_event_queue_empty(queue):
            t = -al_get_time()
            add_time()
            al_clear_to_color(al_map_rgb_f(0, 0, 0))
            redraw()
            t += al_get_time()
            example.direct_speed_measure  = t
            al_flip_display()
            need_redraw = False

        al_wait_for_event(queue, byref(event))

        if event.type == ALLEGRO_EVENT_KEY_CHAR:
            if event.keyboard.keycode == ALLEGRO_KEY_ESCAPE:
               done = True
            elif event.keyboard.keycode == ALLEGRO_KEY_UP:
               add_sprites(1)
            elif event.keyboard.keycode == ALLEGRO_KEY_DOWN:
               remove_sprites(1)
            elif event.keyboard.keycode == ALLEGRO_KEY_LEFT:
               change_size(example.bitmap_size - 1)
            elif event.keyboard.keycode == ALLEGRO_KEY_RIGHT:
               change_size(example.bitmap_size + 1)
            elif event.keyboard.keycode == ALLEGRO_KEY_F1:
               example.show_help ^= 1
            elif event.keyboard.keycode == ALLEGRO_KEY_SPACE:
               example.use_memory_bitmaps ^= 1
               change_size(example.bitmap_size)
            elif event.keyboard.keycode == ALLEGRO_KEY_B:
               example.blending += 1
               if example.blending == 4:
                  example.blending = 0
        elif event.type == ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = True

        elif event.type == ALLEGRO_EVENT_TIMER:
            update()
            need_redraw = True

        elif event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            example.mouse_down = True
            example.last_x = event.mouse.x
            example.last_y = event.mouse.y

        elif event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            fh = al_get_font_line_height(example.font)
            example.mouse_down = False
            if event.mouse.x < 40 and event.mouse.y >= h - fh * 5:
                button = (event.mouse.y - (h - fh * 5)) // fh
                if button == 0:
                    example.use_memory_bitmaps ^= 1
                    change_size(example.bitmap_size)
                if button == 1:
                    example.blending += 1
                    if example.blending == 4:
                        example.blending = 0
                if button == 4:
                    example.show_help ^= 1

        elif event.type == ALLEGRO_EVENT_MOUSE_AXES:
            if example.mouse_down:
                dx = event.mouse.x - example.last_x
                dy = event.mouse.y - example.last_y
                if dy > 4:
                    add_sprites(int(dy / 4))

                if dy < -4:
                    remove_sprites(-int(dy / 4))

                if dx > 4:
                    change_size(example.bitmap_size + dx - 4)

                if dx < -4:
                    change_size(example.bitmap_size + dx + 4)
                
                example.last_x = event.mouse.x
                example.last_y = event.mouse.y

    al_destroy_bitmap(example.bitmap)
    al_uninstall_system()

al_main(main)
