/* Simple networked game example using ENet (http://enet.bespin.org/).
 *
 * You will need enet installed to run this demo.
 *
 * This example is based on http://enet.bespin.org/Tutorial.html
 *
 * This is the server, which you will want to start first. Once the server is
 * running, run ex_enet_client to connect each client.
 */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <enet/enet.h>
#include <allegro5/allegro.h>

#include "common.c"
#include "enet_common.h"

#define PLAYER_SPEED 200 // pixels / second

const int port = 9234;

static float rand01(void) { return rand() / (float)RAND_MAX; }

static int init_player(void)
{
   int i;
   // find the first open player slot
   for (i = 0 ; i < MAX_PLAYER_COUNT ; ++i) {
      if (!players[i].active) {
         // assign a random color and position to this new player
         players[i].active = true;
         players[i].color = al_map_rgb_f(rand01(), rand01(), rand01());
         players[i].x = rand01() * SCREEN_W;
         players[i].y = rand01() * SCREEN_H;

         return i;
      }
   }

   abort_example("Cannot create more than %d clients", MAX_PLAYER_COUNT);
   return -1;
}

static ServerMessage create_join_message(int player_id)
{
   ServerMessage msg_out;
   msg_out.type = PLAYER_JOIN;
   msg_out.player_id = player_id;
   msg_out.color = players[player_id].color;
   msg_out.x = players[player_id].x;
   msg_out.y = players[player_id].y;
   return msg_out;
}

static void send_receive(ENetHost *server)
{
   ClientMessage *msg_in;
   ServerMessage msg_out;
   ENetPacket *packet;
   ENetEvent event;
   int i;

   // Process all messages in queue, exit as soon as it is empty
   while (enet_host_service(server, &event, 0) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_NONE:
            break;
         case ENET_EVENT_TYPE_CONNECT:
            printf("Server: A new client connected from %x:%u.\n",
               event.peer->address.host,
               event.peer->address.port);

            int player_id = init_player();

            // store the id with the peer we can correlate messages from this
            // client with this ID
            event.peer->data = malloc(sizeof(int));
            *(int*)event.peer->data = player_id;

            // notify all clients of the new player
            msg_out = create_join_message(player_id);

            printf("Server: created player #%d at %d,%d\n",
               player_id,
               players[player_id].x,
               players[player_id].y);

            packet = enet_packet_create(&msg_out,
               sizeof(msg_out),
               ENET_PACKET_FLAG_RELIABLE);

            enet_host_broadcast(server, 0, packet);

            // notify new client of all other existing players
            for (i = 0 ; i < MAX_PLAYER_COUNT ; ++i) {
               if (!players[i].active || i == player_id) continue;

               msg_out = create_join_message(i);

               packet = enet_packet_create(&msg_out,
                  sizeof(msg_out),
                  ENET_PACKET_FLAG_RELIABLE);
               enet_peer_send(event.peer, 0, packet);
            }

            break;
         case ENET_EVENT_TYPE_RECEIVE:
            msg_in = (ClientMessage*)event.packet->data;

            // update the player's movement
            player_id = *(int*)event.peer->data;
            players[player_id].dx = msg_in->x;
            players[player_id].dy = msg_in->y;

            /* Clean up the incoming packet now that we're done using it. */
            enet_packet_destroy(event.packet);
            break;
         case ENET_EVENT_TYPE_DISCONNECT:
            printf("Server: client #%d disconnected.\n", *((int*)event.peer->data));

            // notify all other players that a player left
            msg_out.type = PLAYER_LEAVE;
            msg_out.player_id = *(int*)event.peer->data;

            packet = enet_packet_create(&msg_out,
               sizeof(msg_out),
               ENET_PACKET_FLAG_RELIABLE);

            enet_host_broadcast(server, 0, packet);

            /* Reset the peer's server information. */
            free(event.peer->data);
            event.peer->data = NULL;
      }
   }
}

static void update_players(ENetHost *server, float time)
{
   int i;
   for (i = 0 ; i < MAX_PLAYER_COUNT ; ++i) {
      if (!players[i].active) continue;

      players[i].x += players[i].dx * PLAYER_SPEED * time;
      players[i].y += players[i].dy * PLAYER_SPEED * time;

      // notify all clients of this player's new position
      ServerMessage msg;
      msg.type = POSITION_UPDATE;
      msg.player_id = i;
      msg.x = players[i].x;
      msg.y = players[i].y;

      ENetPacket *packet = enet_packet_create(&msg,
         sizeof(ServerMessage),
         ENET_PACKET_FLAG_RELIABLE);

      enet_host_broadcast(server, 0, packet);
   }
}

static ENetHost* create_server(int port)
{
   ENetAddress address;
   ENetHost *server;
   /* Bind the server to the default localhost.     */
   /* A specific host address can be specified by   */
   /* enet_address_set_host (&address, "x.x.x.x"); */
   address.host = ENET_HOST_ANY;
   /* Bind the server to port 1234. */
   address.port = port;
   server = enet_host_create(&address /* the address to bind the server host to */,
      32      /* allow up to 32 clients and/or outgoing connections */,
      2      /* allow up to 2 channels to be used, 0 and 1 */,
      0      /* assume any amount of incoming bandwidth */,
      0      /* assume any amount of outgoing bandwidth */);
   if (server == NULL) {
      fprintf(stderr, "Failed to create the server.\n");
      exit(EXIT_FAILURE);
   }

   return server;
}

int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   double last_time; // time of last update
   double cur_time;  // time of this update
   srand(time(NULL));
   ENetHost * server;
   int port;

   if (argc == 1)
      port = DEFAULT_PORT;
   else if (argc == 2)
      port = atoi(argv[1]);
   else
      abort_example("Usage: %s [portnum]", argv[0]);

   if (!al_init())
      abort_example("Could not init Allegro.\n");

   if (enet_initialize() != 0) {
      fprintf(stderr, "An error occurred while initializing ENet.\n");
      return EXIT_FAILURE;
   }

   timer = al_create_timer(1.0 / FPS); // Run at 30FPS
   queue = al_create_event_queue();

   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   server = create_server(port);

   last_time = al_get_time();

   while(1)
   {
      al_wait_for_event(queue, &event); // Wait for and get an event.

      if (event.type == A5O_EVENT_TIMER) {
         cur_time = al_get_time();
         update_players(server, cur_time - last_time);
         last_time = cur_time;

         send_receive(server);
      }
   }

   enet_host_destroy(server);
   enet_deinitialize();
   return 0;
}

/* vim: set sts=3 sw=3 et: */
