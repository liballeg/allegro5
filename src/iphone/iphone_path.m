#import <Foundation/Foundation.h>
#include <allegro5/internal/aintern_iphone.h>
#include <mach-o/dyld.h>

ALLEGRO_PATH *_al_iphone_get_path(int id)
{
   char str[PATH_MAX];
   NSString *string;
   NSArray *array;
   NSBundle *mainBundle;

   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

   switch (id) {
      case ALLEGRO_USER_HOME_PATH:
         string = NSHomeDirectory();
         break;
      case ALLEGRO_TEMP_PATH:
         string = NSTemporaryDirectory();
         break;
      case ALLEGRO_RESOURCES_PATH:
         mainBundle = [NSBundle mainBundle];
         string = [mainBundle resourcePath];
         break;
      case ALLEGRO_USER_SETTINGS_PATH:
      case ALLEGRO_USER_DATA_PATH:
         array = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            TRUE);
         string = (NSString *)[array objectAtIndex:0];
         break;

      case ALLEGRO_USER_DOCUMENTS_PATH:
         array = NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory,
            NSUserDomainMask,
            TRUE);
         string = (NSString *)[array objectAtIndex:0];
         break;
      case ALLEGRO_EXENAME_PATH: {
         uint32_t size = sizeof(str);
         if (_NSGetExecutablePath(str, &size) != 0) {
            [pool drain];
            return NULL;
         }
         [pool drain];
         return al_create_path(str);
      }
      default:
         [pool drain];
         return NULL;
   }

   sprintf(str, "%s", [string UTF8String]);

   [pool drain];

   return al_create_path_for_directory(str);
}
