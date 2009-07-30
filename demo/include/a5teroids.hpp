#ifndef A5TEROIDS_HPP
#define A5TEROIDS_HPP

#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_vorbis.h"
#include "allegro5/kcm_audio.h"

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

#include "Error.hpp"
#include "Debug.hpp"
//#include "Configuration.hpp"
#include "Resource.hpp"
#include "BitmapResource.hpp"
#include "DisplayResource.hpp"
#include "GenericResource.hpp"
#include "SampleResource.hpp"
#include "StreamResource.hpp"
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

const int BB_W = 800;
const int BB_H = 600;

extern ALLEGRO_VOICE *voice;
extern ALLEGRO_MIXER *mixer;

#endif // A5TEROIDS_HPP

