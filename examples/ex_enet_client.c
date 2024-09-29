/* Simple networked game example using ENet (http://enet.bespin.org/).
 *
 * You will need enet installed to run this demo.
 *
 * This example is based on http://enet.bespin.org/Tutorial.html
 *
 * To try this example, first run ex_enet_server.
 * Then start multiple instances of ex_enet_client.
 */
#include <stdio.h>
#include <stdlib.h>
#include <enet/enet.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"
#include "enet_common.h"

static ENetHost* create_client(void)
{
   ENetHost * client;
   client = enet_host_create(NULL /* create a client host */,
      1 /* only allow 1 outgoing connection */,
      2 /* allow up 2 channels to be used, 0 and 1 */,
      57600 / 8 /* 56K modem with 56 Kbps downstream bandwidth */,
      14400 / 8 /* 56K modem with 14 Kbps upstream bandwidth */);

   if (client == NULL)
      abort_example("Client: Failed to create the client.\n");

   return client;
}

static void disconnect_client(ENetHost *client, ENetPeer *server)
{
   enet_peer_disconnect(server, 0);

   /* Allow up to 3 seconds for the disconnect to succeed
    * and drop any packets received packets.
    */
   ENetEvent event;
   while (enet_host_service (client, &event, 3000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
         case ENET_EVENT_TYPE_DISCONNECT:
            puts("Client: Disconnect succeeded.");
            return;
         case ENET_EVENT_TYPE_NONE:
         case ENET_EVENT_TYPE_CONNECT:
            break;
      }
   }

   // failed to disconnect gracefully, force the connection closed
   enet_peer_reset(server);
}

static ENetPeer* connect_client(ENetHost *client, int port)
{
   ENetAddress address;
   ENetEvent event;
   ENetPeer *server;
   enet_address_set_host(&address, "localhost");
   address.port = port;
   /* Initiate the connection, allocating the two channels 0 and 1. */
   server = enet_host_connect(client, &address, 2, 0);
   if (server == NULL)
      abort_example("Client: No available peers for initiating an ENet connection.\n");

   /* Wait up to 5 seconds for the connection attempt to succeed. */
   if (enet_host_service(client, &event, 5000) > 0 &&
      event.type == ENET_EVENT_TYPE_CONNECT)
   {
      printf("Client: Connected to %x:%u.\n",
         event.peer->address.host,
         event.peer->address.port);
   }
   else
   {
      /* Either the 5 seconds are up or a disconnect event was */
      /* received. Reset the peer in the event the 5 seconds   */
      /* had run out without any significant event.            */
      enet_peer_reset(server);
      abort_example("Client: Connection to server failed.");
   }

   return server;
}

static void send_receive(ENetHost *client)
{
   ENetEvent event;
   ServerMessage *msg;

   // Check if we have any queued incoming messages, but do not wait otherwise.
   // This also sends outgoing messages queued with enet_peer_send.
   while (enet_host_service(client, &event, 0) > 0) {
      // clients only care about incoming packets, they will not receive
      // connect/disconnect events.
      if (event.type == ENET_EVENT_TYPE_RECEIVE) {
         msg = (ServerMessage*)event.packet->data;

         switch (msg->type) {
            case POSITION_UPDATE:
               players[msg->player_id].x = msg->x;
               players[msg->player_id].y = msg->y;
               break;
            case PLAYER_JOIN:
               printf("Client: player #%d joined\n", msg->player_id);
               players[msg->player_id].active = true;
               players[msg->player_id].x = msg->x;
               players[msg->player_id].y = msg->y;
               players[msg->player_id].color = msg->color;
               break;
            case PLAYER_LEAVE:
               printf("Client: player #%d left\n", msg->player_id);
               players[msg->player_id].active = false;
               break;
         }

         /* Clean up the packet now that we're done using it. */
         enet_packet_destroy(event.packet);
      }
   }
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   ENetHost *client;
   ENetPeer *server;
   bool update = true; // when true, update positions and render
   bool done = false;  // when true, client exits
   int dx = 0, dy = 0; // movement direction
   int port = DEFAULT_PORT;
   int i;

   if (argc == 2) {
      port = atoi(argv[1]);
   }
   else if (argc > 2)
      abort_example("Usage: %s [portnum]", argv[0]);


   // --- allegro setup ---
   if (!al_init())
      abort_example("Could not init Allegro.\n");

   init_platform_specific();

   al_install_keyboard();
   al_init_primitives_addon();

   // Create a new display that we can render the image to.
   display = al_create_display(SCREEN_W, SCREEN_H);
   if (!display)
      abort_example("Error creating display\n");

   timer = al_create_timer(1.0 / FPS); // Run at 30FPS
   queue = al_create_event_queue();

   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   // --- enet setup ---
   if (enet_initialize() != 0)
      abort_example("An error occurred while initializing ENet.\n");

   client = create_client();
   server = connect_client(client, port);

   // --- game loop ---
   bool direction_changed = false;
   while (!done) {
      al_wait_for_event(queue, &event); // Wait for and get an event.

      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
         case A5O_EVENT_KEY_DOWN:
            switch (event.keyboard.keycode) {
               case A5O_KEY_UP:
               case A5O_KEY_W: dy -= 1; direction_changed = true; break;
               case A5O_KEY_DOWN:
               case A5O_KEY_S: dy += 1; direction_changed = true; break;
               case A5O_KEY_LEFT:
               case A5O_KEY_A: dx -= 1; direction_changed = true; break;
               case A5O_KEY_RIGHT:
               case A5O_KEY_D: dx += 1; direction_changed = true; break;
            }
            break;
         case A5O_EVENT_KEY_UP:
            switch (event.keyboard.keycode) {
               case A5O_KEY_UP:
               case A5O_KEY_W: dy += 1; direction_changed = true; break;
               case A5O_KEY_DOWN:
               case A5O_KEY_S: dy -= 1; direction_changed = true; break;
               case A5O_KEY_LEFT:
               case A5O_KEY_A: dx += 1; direction_changed = true; break;
               case A5O_KEY_RIGHT:
               case A5O_KEY_D: dx -= 1; direction_changed = true; break;
            }
            break;
         case (A5O_EVENT_TIMER):
            update = true;
            break;
      }

      // update, but only if the event queue is empty
      if (update && al_is_event_queue_empty(queue)) {
         update = false;

         // if player changed direction this frame, notify the server.
         // only check once per frame to stop clients from flooding the server
         if (direction_changed) {
            direction_changed = false;

            ClientMessage msg = { dx, dy };

            ENetPacket *packet = enet_packet_create(&msg,
               sizeof(msg),
               ENET_PACKET_FLAG_RELIABLE);

            enet_peer_send(server, 0, packet);
         }

         // this will send our queued direction message if we have one, and get
         // position updates for other clients
         send_receive(client);

         // draw each player
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         for (i = 0; i < MAX_PLAYER_COUNT; i++) {
            if (!players[i].active) continue;

            int x = players[i].x;
            int y = players[i].y;
            A5O_COLOR color = players[i].color;
            al_draw_filled_circle(x, y, PLAYER_SIZE, color);
         }
         al_flip_display();
      }
   }

   disconnect_client(client, server);
   enet_host_destroy(client);
   enet_deinitialize();
   return 0;
}

/* vim: set sts=3 sw=3 et: */
