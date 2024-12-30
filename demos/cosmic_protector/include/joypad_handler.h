#ifdef USE_JOYPAD

#import "JoypadSDK.h"

@interface joypad_handler : NSObject<JoypadManagerDelegate, JoypadDeviceDelegate>
{
        JoypadManager *joypadManager;

@public
        bool connected, left, right, up, down, ba, bb, bx, by, bl, br;
        bool finding;
}

-(void)start;
-(void)find_devices;
-(void)stop_finding_devices;

@end

#endif

#include "joypad_c.h"
