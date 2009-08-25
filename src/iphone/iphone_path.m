#import <Foundation/Foundation.h>
#include <allegro5/internal/aintern_iphone.h>

ALLEGRO_PATH *_al_iphone_get_path(int id)
{
    char str[PATH_MAX];
    NSString *string;
    NSBundle *mainBundle;
    switch (id) {
        case ALLEGRO_EXENAME_PATH:
        case ALLEGRO_PROGRAM_PATH:
        case ALLEGRO_USER_HOME_PATH:
            strcpy(str, getenv("HOME"));
            return al_create_path_for_dir(str);
        case ALLEGRO_TEMP_PATH:
            string = NSTemporaryDirectory();
            sprintf(str, "%s", [string UTF8String]);
            return al_create_path_for_dir(str);
        case ALLEGRO_SYSTEM_DATA_PATH:
        case ALLEGRO_USER_DATA_PATH:
            mainBundle = [NSBundle mainBundle];
            string = [mainBundle resourcePath];
            sprintf(str, "%s", [string UTF8String]);
            return al_create_path_for_dir(str);
        case ALLEGRO_USER_SETTINGS_PATH:
        case ALLEGRO_SYSTEM_SETTINGS_PATH:
            sprintf(str, "%s/Library/Preferences", getenv("HOME"));
            return al_create_path_for_dir(str);
    }
    return NULL;
}
