#!/usr/bin/env python3
"""
    Allegro Python port game example
    Original Author: Jeroen De Busser
"""
from allegro import *
from random import randint, random
import os

examples_data = os.path.abspath(os.path.join(os.path.dirname(__file__),
    "..", "examples/data"))


class Pallet:
    def __init__(self, x, y, w, h, speed, color):
        self.__initials = (x, y, w, h, speed, color, 0)
        self.reset()

    def reset(self):
        '''Resets this pallet to its starting values '''
        self.x, self.y, self.w, self.h, self.speed, self.color, self.moving =\
            self.__initials

    def update(self):
        self.y += self.moving * self.speed
        if self.y < 0:
            self.y = 0
        elif self.y > al_get_display_height(al_get_current_display()) - self.h:
            self.y = al_get_display_height(al_get_current_display()) - self.h

    def draw(self):
        # Fill
        al_draw_filled_rounded_rectangle(self.x, self.y, self.x + self.w,
            self.y + self.h, 6, 6, self.color)
        # Highlight
        al_draw_filled_rounded_rectangle(self.x + 2, self.y + 2,
            self.x + self.w / 2, self.y + self.h - 5, 4, 4,
            al_map_rgba_f(0.2, 0.2, 0.2, 0.2))


class Player(Pallet):
    def __init__(self, x, y, h, w, speed, keyup, keydown, color):
        self.__initials = (x, y, h, w, speed, keyup, keydown, color)
        Pallet.__init__(self, x, y, h, w, speed, color)
        self.keyup, self.keydown = keyup, keydown

    def handle_event(self, ev):
        if ev.type == A5O_EVENT_KEY_DOWN:
            if ev.keyboard.keycode == self.keyup:
                self.moving = -1
            elif ev.keyboard.keycode == self.keydown:
                self.moving = 1
        elif ev.type == A5O_EVENT_KEY_UP:
            if (ev.keyboard.keycode == self.keyup and self.moving == -1) or\
                (ev.keyboard.keycode == self.keydown and self.moving == 1):
                self.moving = 0


class AI(Pallet):
    def __init__(self, x, y, w, h, speed, color, difficulty, ball):
        Pallet.__init__(self, x, y, w, h, speed, color)
        self.difficulty = difficulty
        self.ball = ball

    def handle_event(self, ev):
        #Only fire on a timer event
        if ev.type == A5O_EVENT_TIMER:
            #Calculate the target y location according to the difficulty level
            if self.difficulty == 1:
                target_y = self.ball.y + self.ball.size / 2
            else:
                # Higher difficulty, so we need to precalculate the position of
                # the ball if it is closer than a certain treshold
                dh = al_get_display_height(al_get_current_display())
                if self.ball.xdir == -1 or abs(self.x - self.ball.x) >\
                        al_get_display_width(al_get_current_display()) * (
                        self.difficulty - 1) / self.difficulty:
                    # If the ball is moving away, return to the center of the
                    # screen
                    target_y = dh / 2
                else:
                    # The ball is moving towards this pallet within its FOV.
                    # Calculate what the y location of the ball will be when it
                    # lands at this pallets x location
                    target_y = self.ball.y + (self.x - self.ball.x) *\
                        self.ball.ydir
                    if target_y < 0:
                        target_y = abs(target_y)
                    elif target_y > dh:
                        target_y = dh - target_y % dh

            #Set the movement
            if target_y > self.y + self.h * 3 / 4:
                self.moving = 1
            elif target_y < self.y + self.h * 1 / 4:
                self.moving = -1
            else:
                self.moving = 0


class Ball:
    def __init__(self, x, y, speed, size, pallets, color):
        self.x, self.y, self.speed, self.size, self.pallets, self.color =\
            x, y, speed, size, pallets, color
        self.xdir = randint(0, 1)
        self.xdir = self.xdir - (self.xdir == 0)
        self.ydir = randint(0, 1)
        self.ydir = self.ydir - (self.ydir == 0)

    def update(self):
        new_x = self.x + self.xdir * self.speed
        new_y = self.y + self.ydir * self.speed
        p = self.pallets[(self.xdir == 1)]  # The pallet this ball is flying to
        if ((new_x <= p.x + p.w and self.xdir == -1) or (new_x + self.size >=
                p.x and self.xdir == 1)) and new_y + self.size >= p.y and\
                new_y <= p.y + p.h:
            # We hit the pallet
            self.xdir = -self.xdir
            self.speed += 0.1
            self.on_pallet_touch(p)
            self.new_x = self.x
        dw = al_get_display_width(al_get_current_display())
        dh = al_get_display_height(al_get_current_display())
        if self.x < 0 or self.x > dw:
            self.on_screen_exit()
        if new_y < 0 or new_y + self.size > dh:
            # We hit a wall
            self.ydir = -self.ydir
            new_y = self.y  # Reset the y value
        self.x = new_x
        self.y = new_y

    def on_pallet_touch(self, pallet):
        """This function should be called on hitting a pallet"""
        pass

    def on_screen_exit(self):
        '''This function is called when this ball exits the screen'''
        pass

    def draw(self):
        # Fill
        al_draw_filled_circle(self.x + self.size / 2, self.y + self.size / 2,
            self.size / 2, self.color)
        # Highlight
        al_draw_filled_circle(self.x + self.size / 4, self.y + self.size / 4,
            self.size / 6, al_map_rgb_f(0.8, 0.9, 0.8))


class Upgrade(Ball):
    def __init__(self, x, y, speed, pallets, color, effect_function):
        Ball.__init__(self, x, y, speed, 10, pallets, color)
        self.function = effect_function
        self.touched = False

    def on_pallet_touch(self, pallet):
        self.touched = True
        self.function(pallet)

    def on_screen_exit(self):
        self.touched = True


def main():
    #Initialisation
    difficulty = int(input("What difficulty do you want to play on? " +
        "Input: 0 for two-player mode, 1-4 for AI difficulty setting.\n"))
    al_install_system(A5O_VERSION_INT, None)

    w, h = 800, 600
    #Make lines draw smoother
    al_set_new_display_option(A5O_SAMPLE_BUFFERS, 1, A5O_SUGGEST)
    al_set_new_display_option(A5O_SAMPLES, 4, A5O_SUGGEST)
    display = al_create_display(w, h)

    al_init_primitives_addon()
    al_install_keyboard()
    al_init_image_addon()
    al_init_font_addon()

    font = al_load_font(os.path.join(examples_data, "fixed_font.tga"), 0, 0)

    finished = False
    need_redraw = True
    FPS = 60
    pallet_w, pallet_h, pallet_speed = 20, 80, 5
    ball_size, ball_speed = 30, 4.6
    players = []
    players.append(Player(0, h / 2 - pallet_h / 2,
        pallet_w, pallet_h, pallet_speed,
        A5O_KEY_W, A5O_KEY_S, al_map_rgb_f(1.0, 0.0, 0.0)))
    ball = Ball(w / 2, h / 2, ball_speed, ball_size, players,
        al_map_rgb_f(0, 1.0, 0))
    if not difficulty:
        players.append(Player(w - pallet_w, h / 2 - pallet_h / 2,
            pallet_w, pallet_h,
         pallet_speed, A5O_KEY_UP, A5O_KEY_DOWN,
         al_map_rgb_f(0.0, 0.0, 1.0)))
    else:
        players.append(AI(w - pallet_w, h / 2 - pallet_h / 2, pallet_w,
            pallet_h, pallet_speed, al_map_rgb_f(0.0, 0.0, 1.0),
            difficulty, ball))

    def upgrade_grow_height(pallet):
        pallet.h *= 1.25

    def upgrade_shrink_height(pallet):
        pallet.h *= 0.8

    def upgrade_increase_speed(pallet):
        pallet.speed *= 1.5

    def upgrade_decrease_speed(pallet):
        pallet.speed /= 1.5

    upgrade_types = [
        (al_map_rgb_f(0.0, 1.0, 0.0), upgrade_grow_height),
        (al_map_rgb_f(1.0, 0.0, 0.0), upgrade_shrink_height),
        (al_map_rgb_f(1.0, 1.0, 1.0), upgrade_increase_speed),
        (al_map_rgb_f(0.3, 0.3, 0.3), upgrade_decrease_speed)]
    upgrade_probability = 0.005

    timer = al_create_timer(1.0 / FPS)

    queue = al_create_event_queue()
    al_register_event_source(queue, al_get_timer_event_source(timer))
    al_register_event_source(queue, al_get_keyboard_event_source())
    al_register_event_source(queue, al_get_display_event_source(display))

    al_start_timer(timer)

    upgrades = []

    score = [0, 0]

    while True:
        if need_redraw and al_is_event_queue_empty(queue):
            al_clear_to_color(al_map_rgb_f(0, 0, 0))
            for player in players:
                player.draw()
            ball.draw()
            for upgrade in upgrades:
                upgrade.draw()
            if not finished:
                al_draw_text(font, al_map_rgb_f(1, 1, 1), w / 2, 10,
                    A5O_ALIGN_CENTRE, "Player 1: use W and S to move, " +
                    "Player 2: use the up and down arrow keys.")
            else:
                al_draw_text(font, al_map_rgb_f(1, 1, 1), w / 2, 10,
                    A5O_ALIGN_CENTRE, "Press R to reset, ESC to exit.")
            al_draw_text(font, al_map_rgb_f(1, 1, 1), w / 2, h - 20,
                A5O_ALIGN_CENTRE, "{0} - {1}".format(*score))
            al_flip_display()
            need_redraw = False

        ev = A5O_EVENT()
        al_wait_for_event(queue, byref(ev))

        if ev.type == A5O_EVENT_TIMER:
            for player in players:
                player.update()
            ball.update()
            if ball.x + ball.size < 0 or ball.x > w:
                finished = True
                score[ball.x < 0] += 1
                al_stop_timer(timer)
            for upgrade in upgrades:
                upgrade.update()
                if upgrade.touched:
                    upgrades.remove(upgrade)
            if random() < upgrade_probability:
                i = randint(0, len(upgrade_types) - 1)
                upgrades.append(Upgrade(w / 2, h / 2, ball_speed,
                    players, upgrade_types[i][0], upgrade_types[i][1]))
            need_redraw = True
        elif (ev.type == A5O_EVENT_KEY_DOWN and
                ev.keyboard.keycode == A5O_KEY_ESCAPE) or\
                ev.type == A5O_EVENT_DISPLAY_CLOSE:
            break
        elif ev.type == A5O_EVENT_KEY_DOWN and\
                ev.keyboard.keycode == A5O_KEY_R:
            ball.x = w / 2
            ball.y = h / 2
            ball.speed = ball_speed
            upgrades = []
            for player in players:
                player.reset()
            al_start_timer(timer)
            finished = False

        for player in players:
            player.handle_event(ev)

    al_uninstall_system()


if __name__ == '__main__':
    #Only start the game when this file is executed directly.
    al_main(main)
