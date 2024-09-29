#include <allegro5/allegro.h>

#ifdef A5O_IPHONE

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "cosmic_protector_objc.h"

bool isMultitaskingSupported(void)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	char buf[100];
	strcpy(buf, [[[UIDevice currentDevice] systemVersion] UTF8String]);
	if (atof(buf) < 4.0) return false;
	
	[pool drain];
	
	return [[UIDevice currentDevice] isMultitaskingSupported];
}
#endif
