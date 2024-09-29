#ifndef ENET_COMMON_H
#define ENET_COMMON_H

#include <allegro5/allegro.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define FPS 30           // framerate
#define PLAYER_SIZE 16   // radius of player circle
#define PLAYER_SPEED 200 // movement rate of player in pixels/sec
#define MAX_PLAYER_COUNT 32
#define DEFAULT_PORT 9234

typedef enum
{
    PLAYER_JOIN,
    PLAYER_LEAVE,
    POSITION_UPDATE,
} MESSAGE_TYPE;

// message sent from client to server
typedef struct
{
    int x; // requested x movement (-1, 0, or 1)
    int y; // requested y movement (-1, 0, or 1)
} ClientMessage;

// message sent from server to client
typedef struct
{
    int player_id;
    MESSAGE_TYPE type;
    int x;               // current position (x)
    int y;               // current position (y)
    A5O_COLOR color; // valid when type == PLAYER_JOIN
} ServerMessage;

// storage for all players
struct
{
   bool active;
   int x, y;   // current position
   int dx, dy; // direction of movemnt
   A5O_COLOR color;
} players[MAX_PLAYER_COUNT];

#endif
