package org.liballeg.android;

final class Key
{
   /* keycodes.h */
   static final int ALLEGRO_KEY_A           = 1;
   static final int ALLEGRO_KEY_B           = 2;
   static final int ALLEGRO_KEY_C           = 3;
   static final int ALLEGRO_KEY_D           = 4;
   static final int ALLEGRO_KEY_E           = 5;
   static final int ALLEGRO_KEY_F           = 6;
   static final int ALLEGRO_KEY_G           = 7;
   static final int ALLEGRO_KEY_H           = 8;
   static final int ALLEGRO_KEY_I           = 9;
   static final int ALLEGRO_KEY_J           = 10;
   static final int ALLEGRO_KEY_K           = 11;
   static final int ALLEGRO_KEY_L           = 12;
   static final int ALLEGRO_KEY_M           = 13;
   static final int ALLEGRO_KEY_N           = 14;
   static final int ALLEGRO_KEY_O           = 15;
   static final int ALLEGRO_KEY_P           = 16;
   static final int ALLEGRO_KEY_Q           = 17;
   static final int ALLEGRO_KEY_R           = 18;
   static final int ALLEGRO_KEY_S           = 19;
   static final int ALLEGRO_KEY_T           = 20;
   static final int ALLEGRO_KEY_U           = 21;
   static final int ALLEGRO_KEY_V           = 22;
   static final int ALLEGRO_KEY_W           = 23;
   static final int ALLEGRO_KEY_X           = 24;
   static final int ALLEGRO_KEY_Y           = 25;
   static final int ALLEGRO_KEY_Z           = 26;

   static final int ALLEGRO_KEY_0           = 27;
   static final int ALLEGRO_KEY_1           = 28;
   static final int ALLEGRO_KEY_2           = 29;
   static final int ALLEGRO_KEY_3           = 30;
   static final int ALLEGRO_KEY_4           = 31;
   static final int ALLEGRO_KEY_5           = 32;
   static final int ALLEGRO_KEY_6           = 33;
   static final int ALLEGRO_KEY_7           = 34;
   static final int ALLEGRO_KEY_8           = 35;
   static final int ALLEGRO_KEY_9           = 36;

   static final int ALLEGRO_KEY_PAD_0       = 37;
   static final int ALLEGRO_KEY_PAD_1       = 38;
   static final int ALLEGRO_KEY_PAD_2       = 39;
   static final int ALLEGRO_KEY_PAD_3       = 40;
   static final int ALLEGRO_KEY_PAD_4       = 41;
   static final int ALLEGRO_KEY_PAD_5       = 42;
   static final int ALLEGRO_KEY_PAD_6       = 43;
   static final int ALLEGRO_KEY_PAD_7       = 44;
   static final int ALLEGRO_KEY_PAD_8       = 45;
   static final int ALLEGRO_KEY_PAD_9       = 46;

   static final int ALLEGRO_KEY_F1          = 47;
   static final int ALLEGRO_KEY_F2          = 48;
   static final int ALLEGRO_KEY_F3          = 49;
   static final int ALLEGRO_KEY_F4          = 50;
   static final int ALLEGRO_KEY_F5          = 51;
   static final int ALLEGRO_KEY_F6          = 52;
   static final int ALLEGRO_KEY_F7          = 53;
   static final int ALLEGRO_KEY_F8          = 54;
   static final int ALLEGRO_KEY_F9          = 55;
   static final int ALLEGRO_KEY_F10         = 56;
   static final int ALLEGRO_KEY_F11         = 57;
   static final int ALLEGRO_KEY_F12         = 58;

   static final int ALLEGRO_KEY_ESCAPE      = 59;
   static final int ALLEGRO_KEY_TILDE       = 60;
   static final int ALLEGRO_KEY_MINUS       = 61;
   static final int ALLEGRO_KEY_EQUALS      = 62;
   static final int ALLEGRO_KEY_BACKSPACE   = 63;
   static final int ALLEGRO_KEY_TAB         = 64;
   static final int ALLEGRO_KEY_OPENBRACE   = 65;
   static final int ALLEGRO_KEY_CLOSEBRACE  = 66;
   static final int ALLEGRO_KEY_ENTER       = 67;
   static final int ALLEGRO_KEY_SEMICOLON   = 68;
   static final int ALLEGRO_KEY_QUOTE       = 69;
   static final int ALLEGRO_KEY_BACKSLASH   = 70;
   static final int ALLEGRO_KEY_BACKSLASH2  = 71; /* DirectInput calls this DIK_OEM_102: "< > | on UK/Germany keyboards" */
   static final int ALLEGRO_KEY_COMMA       = 72;
   static final int ALLEGRO_KEY_FULLSTOP    = 73;
   static final int ALLEGRO_KEY_SLASH       = 74;
   static final int ALLEGRO_KEY_SPACE       = 75;

   static final int ALLEGRO_KEY_INSERT      = 76;
   static final int ALLEGRO_KEY_DELETE      = 77;
   static final int ALLEGRO_KEY_HOME        = 78;
   static final int ALLEGRO_KEY_END         = 79;
   static final int ALLEGRO_KEY_PGUP        = 80;
   static final int ALLEGRO_KEY_PGDN        = 81;
   static final int ALLEGRO_KEY_LEFT        = 82;
   static final int ALLEGRO_KEY_RIGHT       = 83;
   static final int ALLEGRO_KEY_UP          = 84;
   static final int ALLEGRO_KEY_DOWN        = 85;

   static final int ALLEGRO_KEY_PAD_SLASH   = 86;
   static final int ALLEGRO_KEY_PAD_ASTERISK= 87;
   static final int ALLEGRO_KEY_PAD_MINUS   = 88;
   static final int ALLEGRO_KEY_PAD_PLUS    = 89;
   static final int ALLEGRO_KEY_PAD_DELETE  = 90;
   static final int ALLEGRO_KEY_PAD_ENTER   = 91;

   static final int ALLEGRO_KEY_PRINTSCREEN = 92;
   static final int ALLEGRO_KEY_PAUSE       = 93;

   static final int ALLEGRO_KEY_ABNT_C1     = 94;
   static final int ALLEGRO_KEY_YEN         = 95;
   static final int ALLEGRO_KEY_KANA        = 96;
   static final int ALLEGRO_KEY_CONVERT     = 97;
   static final int ALLEGRO_KEY_NOCONVERT   = 98;
   static final int ALLEGRO_KEY_AT          = 99;
   static final int ALLEGRO_KEY_CIRCUMFLEX  = 100;
   static final int ALLEGRO_KEY_COLON2      = 101;
   static final int ALLEGRO_KEY_KANJI       = 102;

   static final int ALLEGRO_KEY_PAD_EQUALS  = 103;   /* MacOS X */
   static final int ALLEGRO_KEY_BACKQUOTE   = 104;   /* MacOS X */
   static final int ALLEGRO_KEY_SEMICOLON2  = 105;   /* MacOS X -- TODO: ask lillo what this should be */
   static final int ALLEGRO_KEY_COMMAND     = 106;   /* MacOS X */
   static final int ALLEGRO_KEY_BACK        = 107;
   static final int ALLEGRO_KEY_VOLUME_UP   = 108;
   static final int ALLEGRO_KEY_VOLUME_DOWN = 109;

   /* Some more standard Android keys.
    * These happen to be the ones used by the Xperia Play.
    */
   static final int ALLEGRO_KEY_SEARCH      = 110;
   static final int ALLEGRO_KEY_DPAD_CENTER = 111;
   static final int ALLEGRO_KEY_BUTTON_X    = 112;
   static final int ALLEGRO_KEY_BUTTON_Y    = 113;
   static final int ALLEGRO_KEY_DPAD_UP     = 114;
   static final int ALLEGRO_KEY_DPAD_DOWN   = 115;
   static final int ALLEGRO_KEY_DPAD_LEFT   = 116;
   static final int ALLEGRO_KEY_DPAD_RIGHT  = 117;
   static final int ALLEGRO_KEY_SELECT      = 118;
   static final int ALLEGRO_KEY_START       = 119;
   static final int ALLEGRO_KEY_BUTTON_L1   = 120;
   static final int ALLEGRO_KEY_BUTTON_R1   = 121;
   static final int ALLEGRO_KEY_BUTTON_L2   = 122;
   static final int ALLEGRO_KEY_BUTTON_R2   = 123;
   static final int ALLEGRO_KEY_BUTTON_A    = 124;
   static final int ALLEGRO_KEY_BUTTON_B    = 125;
   static final int ALLEGRO_KEY_THUMBL      = 126;
   static final int ALLEGRO_KEY_THUMBR      = 127;

   static final int ALLEGRO_KEY_UNKNOWN     = 128;

   /* All codes up to before ALLEGRO_KEY_MODIFIERS can be freely
    * assigned as additional unknown keys, like various multimedia
    * and application keys keyboards may have.
    */

   static final int ALLEGRO_KEY_MODIFIERS   = 215;

   static final int ALLEGRO_KEY_LSHIFT	    = 215;
   static final int ALLEGRO_KEY_RSHIFT      = 216;
   static final int ALLEGRO_KEY_LCTRL       = 217;
   static final int ALLEGRO_KEY_RCTRL       = 218;
   static final int ALLEGRO_KEY_ALT         = 219;
   static final int ALLEGRO_KEY_ALTGR       = 220;
   static final int ALLEGRO_KEY_LWIN        = 221;
   static final int ALLEGRO_KEY_RWIN        = 222;
   static final int ALLEGRO_KEY_MENU        = 223;
   static final int ALLEGRO_KEY_SCROLLLOCK  = 224;
   static final int ALLEGRO_KEY_NUMLOCK     = 225;
   static final int ALLEGRO_KEY_CAPSLOCK    = 226;

   static final int ALLEGRO_KEY_MAX	    = 227;

   protected static int[] keyMap = {
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_UNKNOWN
      ALLEGRO_KEY_LEFT,        // KeyEvent.KEYCODE_SOFT_LEFT
      ALLEGRO_KEY_RIGHT,       // KeyEvent.KEYCODE_SOFT_RIGHT
      ALLEGRO_KEY_HOME,        // KeyEvent.KEYCODE_HOME
      ALLEGRO_KEY_BACK,        // KeyEvent.KEYCODE_BACK
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CALL
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENDCALL
      ALLEGRO_KEY_0,           // KeyEvent.KEYCODE_0
      ALLEGRO_KEY_1,           // KeyEvent.KEYCODE_1
      ALLEGRO_KEY_2,           // KeyEvent.KEYCODE_2
      ALLEGRO_KEY_3,           // KeyEvent.KEYCODE_3
      ALLEGRO_KEY_4,           // KeyEvent.KEYCODE_4
      ALLEGRO_KEY_5,           // KeyEvent.KEYCODE_5 
      ALLEGRO_KEY_6,           // KeyEvent.KEYCODE_6 
      ALLEGRO_KEY_7,           // KeyEvent.KEYCODE_7 
      ALLEGRO_KEY_8,           // KeyEvent.KEYCODE_8 
      ALLEGRO_KEY_9,           // KeyEvent.KEYCODE_9 
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_STAR
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POUND
      ALLEGRO_KEY_UP,          // KeyEvent.KEYCODE_DPAD_UP
      ALLEGRO_KEY_DOWN,        // KeyEvent.KEYCODE_DPAD_DOWN
      ALLEGRO_KEY_LEFT,        // KeyEvent.KEYCODE_DPAD_LEFT
      ALLEGRO_KEY_RIGHT,       // KeyEvent.KEYCODE_DPAD_RIGHT
      ALLEGRO_KEY_ENTER,       // KeyEvent.KEYCODE_DPAD_CENTER
      ALLEGRO_KEY_VOLUME_UP,   // KeyEvent.KEYCODE_VOLUME_UP
      ALLEGRO_KEY_VOLUME_DOWN, // KeyEvent.KEYCODE_VOLUME_DOWN
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POWER
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CAMERA
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CLEAR
      ALLEGRO_KEY_A,           // KeyEvent.KEYCODE_A
      ALLEGRO_KEY_B,           // KeyEvent.KEYCODE_B
      ALLEGRO_KEY_C,           // KeyEvent.KEYCODE_B
      ALLEGRO_KEY_D,           // KeyEvent.KEYCODE_D
      ALLEGRO_KEY_E,           // KeyEvent.KEYCODE_E
      ALLEGRO_KEY_F,           // KeyEvent.KEYCODE_F
      ALLEGRO_KEY_G,           // KeyEvent.KEYCODE_G
      ALLEGRO_KEY_H,           // KeyEvent.KEYCODE_H
      ALLEGRO_KEY_I,           // KeyEvent.KEYCODE_I
      ALLEGRO_KEY_J,           // KeyEvent.KEYCODE_J
      ALLEGRO_KEY_K,           // KeyEvent.KEYCODE_K 
      ALLEGRO_KEY_L,           // KeyEvent.KEYCODE_L
      ALLEGRO_KEY_M,           // KeyEvent.KEYCODE_M
      ALLEGRO_KEY_N,           // KeyEvent.KEYCODE_N
      ALLEGRO_KEY_O,           // KeyEvent.KEYCODE_O
      ALLEGRO_KEY_P,           // KeyEvent.KEYCODE_P
      ALLEGRO_KEY_Q,           // KeyEvent.KEYCODE_Q
      ALLEGRO_KEY_R,           // KeyEvent.KEYCODE_R
      ALLEGRO_KEY_S,           // KeyEvent.KEYCODE_S
      ALLEGRO_KEY_T,           // KeyEvent.KEYCODE_T
      ALLEGRO_KEY_U,           // KeyEvent.KEYCODE_U
      ALLEGRO_KEY_V,           // KeyEvent.KEYCODE_V
      ALLEGRO_KEY_W,           // KeyEvent.KEYCODE_W
      ALLEGRO_KEY_X,           // KeyEvent.KEYCODE_X
      ALLEGRO_KEY_Y,           // KeyEvent.KEYCODE_Y
      ALLEGRO_KEY_Z,           // KeyEvent.KEYCODE_Z
      ALLEGRO_KEY_COMMA,       // KeyEvent.KEYCODE_COMMA 
      ALLEGRO_KEY_FULLSTOP,    // KeyEvent.KEYCODE_PERIOD 
      ALLEGRO_KEY_ALT,         // KeyEvent.KEYCODE_ALT_LEFT
      ALLEGRO_KEY_ALTGR,       // KeyEvent.KEYCODE_ALT_RIGHT
      ALLEGRO_KEY_LSHIFT,      // KeyEvent.KEYCODE_SHIFT_LEFT
      ALLEGRO_KEY_RSHIFT,      // KeyEvent.KEYCODE_SHIFT_RIGHT
      ALLEGRO_KEY_TAB,         // KeyEvent.KEYCODE_TAB
      ALLEGRO_KEY_SPACE,       // KeyEvent.KEYCODE_SPACE
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SYM
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_EXPLORER
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENVELOPE
      ALLEGRO_KEY_ENTER,       // KeyEvent.KEYCODE_ENTER
      ALLEGRO_KEY_BACKSPACE,   // KeyEvent.KEYCODE_DEL
      ALLEGRO_KEY_TILDE,       // KeyEvent.KEYCODE_GRAVE
      ALLEGRO_KEY_MINUS,       // KeyEvent.KEYCODE_MINUS
      ALLEGRO_KEY_EQUALS,      // KeyEvent.KEYCODE_EQUALS
      ALLEGRO_KEY_OPENBRACE,   // KeyEvent.KEYCODE_LEFT_BRACKET
      ALLEGRO_KEY_CLOSEBRACE,  // KeyEvent.KEYCODE_RIGHT_BRACKET
      ALLEGRO_KEY_BACKSLASH,   // KeyEvent.KEYCODE_BACKSLASH
      ALLEGRO_KEY_SEMICOLON,   // KeyEvent.KEYCODE_SEMICOLON
      ALLEGRO_KEY_QUOTE,       // KeyEvent.KEYCODE_APOSTROPHY
      ALLEGRO_KEY_SLASH,       // KeyEvent.KEYCODE_SLASH
      ALLEGRO_KEY_AT,          // KeyEvent.KEYCODE_AT
      ALLEGRO_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_HEADSETHOOK
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_FOCUS
      ALLEGRO_KEY_PAD_PLUS,    // KeyEvent.KEYCODE_PLUS
      ALLEGRO_KEY_MENU,        // KeyEvent.KEYCODE_MENU
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_NOTIFICATION
      ALLEGRO_KEY_SEARCH,      // KeyEvent.KEYCODE_SEARCH
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_STOP
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_NEXT
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PREVIOUS
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_REWIND
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_FAST_FORWARD
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MUTE
      ALLEGRO_KEY_PGUP,        // KeyEvent.KEYCODE_PAGE_UP
      ALLEGRO_KEY_PGDN,        // KeyEvent.KEYCODE_PAGE_DOWN
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_PICTSYMBOLS
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SWITCH_CHARSET
      ALLEGRO_KEY_BUTTON_A,    // KeyEvent.KEYCODE_BUTTON_A
      ALLEGRO_KEY_BUTTON_B,    // KeyEvent.KEYCODE_BUTTON_B
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_C
      ALLEGRO_KEY_BUTTON_X,    // KeyEvent.KEYCODE_BUTTON_X
      ALLEGRO_KEY_BUTTON_Y,    // KeyEvent.KEYCODE_BUTTON_Y
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_Z
      ALLEGRO_KEY_BUTTON_L1,   // KeyEvent.KEYCODE_BUTTON_L1
      ALLEGRO_KEY_BUTTON_R1,   // KeyEvent.KEYCODE_BUTTON_R1
      ALLEGRO_KEY_BUTTON_L2,   // KeyEvent.KEYCODE_BUTTON_L2
      ALLEGRO_KEY_BUTTON_R2,   // KeyEvent.KEYCODE_BUTTON_R2
      ALLEGRO_KEY_THUMBL,      // KeyEvent.KEYCODE_BUTTON_THUMBL
      ALLEGRO_KEY_THUMBR,      // KeyEvent.KEYCODE_BUTTON_THUMBR
      ALLEGRO_KEY_START,       // KeyEvent.KEYCODE_BUTTON_START
      ALLEGRO_KEY_SELECT,      // KeyEvent.KEYCODE_BUTTON_SELECT
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_MODE
      ALLEGRO_KEY_ESCAPE,      // KeyEvent.KEYCODE_ESCAPE (111)
      ALLEGRO_KEY_DELETE,      // KeyEvent.KEYCODE_FORWARD_DELETE (112)
      ALLEGRO_KEY_LCTRL,       // KeyEvent.KEYCODE_CTRL_LEFT (113)
      ALLEGRO_KEY_RCTRL,       // KeyEvent.KEYCODE_CONTROL_RIGHT (114)
      ALLEGRO_KEY_CAPSLOCK,    // KeyEvent.KEYCODE_CAPS_LOCK (115)
      ALLEGRO_KEY_SCROLLLOCK,  // KeyEvent.KEYCODE_SCROLL_LOCK (116)
      ALLEGRO_KEY_LWIN,        // KeyEvent.KEYCODE_META_LEFT (117)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (118)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (119)
      ALLEGRO_KEY_PRINTSCREEN, // KeyEvent.KEYCODE_SYSRQ (120)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (121)
      ALLEGRO_KEY_HOME,        // KeyEvent.KEYCODE_MOVE_HOME (122)
      ALLEGRO_KEY_END,         // KeyEvent.KEYCODE_MOVE_END (123)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (124)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (125)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (126)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (127)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (128)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (129)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (130)
      ALLEGRO_KEY_F1,          // KeyEvent.KEYCODE_ (131)
      ALLEGRO_KEY_F2,          // KeyEvent.KEYCODE_ (132)
      ALLEGRO_KEY_F3,          // KeyEvent.KEYCODE_ (133)
      ALLEGRO_KEY_F4,          // KeyEvent.KEYCODE_ (134)
      ALLEGRO_KEY_F5,          // KeyEvent.KEYCODE_ (135)
      ALLEGRO_KEY_F6,          // KeyEvent.KEYCODE_ (136)
      ALLEGRO_KEY_F7,          // KeyEvent.KEYCODE_ (137)
      ALLEGRO_KEY_F8,          // KeyEvent.KEYCODE_ (138)
      ALLEGRO_KEY_F9,          // KeyEvent.KEYCODE_ (139)
      ALLEGRO_KEY_F10,         // KeyEvent.KEYCODE_ (140)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (141)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (142)
      ALLEGRO_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM_LOCK (143)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (144)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (145)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (146)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (147)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (148)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (149)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (150)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (151)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (152)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (153)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (154)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (155)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (156)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (157)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (158)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (159)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (160)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (161)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (162)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (163)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (164)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (165)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (166)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (167)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (168)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (169)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (170)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (171)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (172)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (173)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (174)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (175)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (176)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (177)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (178)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (179)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (180)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (181)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (182)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (183)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (184)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (185)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (186)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (187)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (188)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (189)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (190)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (191)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (192)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (193)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (194)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (195)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (196)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (197)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (198)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (199)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (200)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (201)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (202)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (203)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (204)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (205)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (206)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (207)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (208)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (209)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (210)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (211)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (212)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (213)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (214)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (215)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (216)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (217)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (218)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (219)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (220)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (221)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (222)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (223)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (224)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (225)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (226)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (227)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (228)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (229)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (230)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (231)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (232)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (233)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (234)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (235)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (236)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (237)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (238)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (239)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (240)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (241)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (242)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (243)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (244)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (245)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (246)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (247)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (248)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (249)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (250)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (251)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (252)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (253)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (254)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (255)
   };

   /* Return Allegro key code for Android key code. */
   static int alKey(int keyCode) {
      return keyMap[keyCode];
   }
}

/* vim: set sts=3 sw=3 et: */
