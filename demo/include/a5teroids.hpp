#ifndef A5TEROIDS_HPP
#define A5TEROIDS_HPP

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#ifdef ALLEGRO_UNIX
#define MAX_PATH 5000
#endif
#ifdef ALLEGRO_MACOSX

#endif
#if defined(ALLEGRO_MINGW32) || defined(ALLEGRO_MSVC) || defined(ALLEGRO_BCC32)
#include "allegro5/winalleg.h"
#ifndef _WIN32_IE
#define _WIN32_IE 0x400
#endif
#include <shlobj.h>
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

#include "Error.hpp"
#include "Debug.hpp"
//#include "Configuration.hpp"
#include "Resource.hpp"
#include "BitmapResource.hpp"
#include "DisplayResource.hpp"
#include "GenericResource.hpp"
#include "SampleResource.hpp"
#include "ResourceManager.hpp"
#include "Misc.hpp"
//#include "Script.hpp"
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
#include "Enemy.hpp"
#include "Asteroid.hpp"
#include "LargeAsteroid.hpp"
#include "MediumAsteroid.hpp"
#include "SmallAsteroid.hpp"
#include "UFO.hpp"
#include "Widget.hpp"
#include "ButtonWidget.hpp"
#include "gui.hpp"
#include "logic.hpp"

const int BB_W = 640;
const int BB_H = 480;

#endif // A5TEROIDS_HPP

