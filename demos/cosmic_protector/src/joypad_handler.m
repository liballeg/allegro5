#import "joypad_handler.h"

#ifdef USE_JOYPAD

#import "JoypadSDK.h"

static joypad_handler *joypad;

@implementation joypad_handler

- (id)init
{
   self = [super init];
   if (self) {
   }

   return self;
}

-(void)start
{
   finding = false;
   joypadManager = [[JoypadManager alloc] init];
   [joypadManager setDelegate:self];

   JoypadControllerLayout *layout = [JoypadControllerLayout snesLayout];
   [layout setName:@"cosmic_protector"];
   [joypadManager setControllerLayout:layout];

   [joypadManager setMaxPlayerCount:1];

   connected = left = right = up = down = ba = bb = bx = by = bl = br = false;
}

-(void)find_devices
{
   finding = true;
   if (connected) return;
   [joypadManager stopFindingDevices];
   [joypadManager startFindingDevices];
}

-(void)stop_finding_devices
{
   finding = false;
   [joypadManager stopFindingDevices];
}

-(void)joypadManager:(JoypadManager *)manager didFindDevice:(JoypadDevice *)device previouslyConnected:(BOOL)prev
{
   [manager stopFindingDevices];
   [manager connectToDevice:device asPlayer:1];
}

-(void)joypadManager:(JoypadManager *)manager didLoseDevice:(JoypadDevice *)device;
{
}

-(void)joypadManager:(JoypadManager *)manager deviceDidConnect:(JoypadDevice *)device player:(unsigned int)player
{
   [device setDelegate:self];
   connected = true;
}

-(void)joypadManager:(JoypadManager *)manager deviceDidDisconnect:(JoypadDevice *)device player:(unsigned int)player
{
   connected = false;
   left = right = up = down = ba = bb = bx = by = bl = br = false;
   if (finding) [self find_devices];
}

-(void)joypadDevice:(JoypadDevice *)device didAccelerate:(JoypadAcceleration)accel
{
}

-(void)joypadDevice:(JoypadDevice *)device dPad:(JoyInputIdentifier)dpad buttonUp:(JoyDpadButton)dpadButton
{
   if (dpadButton == kJoyDpadButtonUp) {
      up = false;
   }
   else if (dpadButton == kJoyDpadButtonDown) {
      down = false;
   }
   else if (dpadButton == kJoyDpadButtonLeft) {
      left = false;
   }
   else if (dpadButton == kJoyDpadButtonRight) {
      right = false;
   }
}

-(void)joypadDevice:(JoypadDevice *)device dPad:(JoyInputIdentifier)dpad buttonDown:(JoyDpadButton)dpadButton
{
   if (dpadButton == kJoyDpadButtonUp) {
      up = true;
   }
   else if (dpadButton == kJoyDpadButtonDown) {
      down = true;
   }
   else if (dpadButton == kJoyDpadButtonLeft) {
      left = true;
   }
   else if (dpadButton == kJoyDpadButtonRight) {
      right = true;
   }
}

-(void)joypadDevice:(JoypadDevice *)device buttonUp:(JoyInputIdentifier)button
{
   if (button == kJoyInputAButton) {
      ba = false;
   }
   else if (button == kJoyInputBButton) {
      bb = false;
   }
   else if (button == kJoyInputXButton) {
      bx = false;
   }
   else if (button == kJoyInputYButton) {
      by = false;
   }
   else if (button == kJoyInputLButton) {
      bl = false;
   }
   else if (button == kJoyInputRButton) {
      br = false;
   }
}

-(void)joypadDevice:(JoypadDevice *)device buttonDown:(JoyInputIdentifier)button
{
   if (button == kJoyInputAButton) {
      ba = true;
   }
   else if (button == kJoyInputBButton) {
      bb = true;
   }
   else if (button == kJoyInputXButton) {
      bx = true;
   }
   else if (button == kJoyInputYButton) {
      by = true;
   }
   else if (button == kJoyInputLButton) {
      bl = true;
   }
   else if (button == kJoyInputRButton) {
      br = true;
   }
}

-(void)joypadDevice:(JoypadDevice *)device analogStick:(JoyInputIdentifier)stick didMove:(JoypadStickPosition)newPosition
{
}

@end

void joypad_start(void)
{
   joypad = [[joypad_handler alloc] init];
   [joypad performSelectorOnMainThread: @selector(start) withObject:nil waitUntilDone:YES];
}

void joypad_find(void)
{
   [joypad performSelectorOnMainThread: @selector(find_devices) withObject:nil waitUntilDone:YES];
}

void joypad_stop_finding(void)
{
   [joypad performSelectorOnMainThread: @selector(stop_finding_devices) withObject:nil waitUntilDone:YES];
}

void get_joypad_state(bool *u, bool *d, bool *l, bool *r, bool *b, bool *esc)
{
   if (!joypad || !joypad->connected) {
      *u = *d = *l = *r = *b = *esc = false;
      return;
   }

   *u = joypad->up;
   *d = joypad->down;
   *l = joypad->left;
   *r = joypad->right;
   *b = joypad->bb;
   *esc = joypad->bx;
}

bool is_joypad_connected(void)
{
   return joypad && joypad->connected;
}

#endif // USE_JOYPAD
