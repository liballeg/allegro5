#ifndef COSMIC_PROTECTOR_HPP
#define COSMIC_PROTECTOR_HPP

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"

#ifdef ALLEGRO_UNIX
#define MAX_PATH 5000
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vector>
#include <list>
#include <math.h>
#include <time.h>

#include "Debug.hpp"
#include "Resource.hpp"
#include "BitmapResource.hpp"
#include "DisplayResource.hpp"
#include "FontResource.hpp"
#include "Game.hpp"
#include "SampleResource.hpp"
#include "StreamResource.hpp"
#include "ResourceManager.hpp"
#include "Input.hpp"
#include "sound.hpp"
#include "collision.hpp"
#include "Entity.hpp"
#include "Weapon.hpp"
#include "Bullet.hpp"
#include "SmallBullet.hpp"
#include "LargeBullet.hpp"
#include "LargeSlowBullet.hpp"
#include "PowerUp.hpp"
#include "Player.hpp"
#include "Explosion.hpp"
#include "render.hpp"
#include "Wave.hpp"
#include "Asteroid.hpp"
#include "LargeAsteroid.hpp"
#include "MediumAsteroid.hpp"
#include "SmallAsteroid.hpp"
#include "UFO.hpp"
#include "Widget.hpp"
#include "ButtonWidget.hpp"
#include "gui.hpp"
#include "logic.hpp"

const int BB_W = 800;
const int BB_H = 600;

#endif // COSMIC_PROTECTOR_HPP

