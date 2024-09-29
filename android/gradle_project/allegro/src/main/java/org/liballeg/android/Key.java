package org.liballeg.android;

final class Key
{
   /* keycodes.h */
   static final int A5O_KEY_A           = 1;
   static final int A5O_KEY_B           = 2;
   static final int A5O_KEY_C           = 3;
   static final int A5O_KEY_D           = 4;
   static final int A5O_KEY_E           = 5;
   static final int A5O_KEY_F           = 6;
   static final int A5O_KEY_G           = 7;
   static final int A5O_KEY_H           = 8;
   static final int A5O_KEY_I           = 9;
   static final int A5O_KEY_J           = 10;
   static final int A5O_KEY_K           = 11;
   static final int A5O_KEY_L           = 12;
   static final int A5O_KEY_M           = 13;
   static final int A5O_KEY_N           = 14;
   static final int A5O_KEY_O           = 15;
   static final int A5O_KEY_P           = 16;
   static final int A5O_KEY_Q           = 17;
   static final int A5O_KEY_R           = 18;
   static final int A5O_KEY_S           = 19;
   static final int A5O_KEY_T           = 20;
   static final int A5O_KEY_U           = 21;
   static final int A5O_KEY_V           = 22;
   static final int A5O_KEY_W           = 23;
   static final int A5O_KEY_X           = 24;
   static final int A5O_KEY_Y           = 25;
   static final int A5O_KEY_Z           = 26;

   static final int A5O_KEY_0           = 27;
   static final int A5O_KEY_1           = 28;
   static final int A5O_KEY_2           = 29;
   static final int A5O_KEY_3           = 30;
   static final int A5O_KEY_4           = 31;
   static final int A5O_KEY_5           = 32;
   static final int A5O_KEY_6           = 33;
   static final int A5O_KEY_7           = 34;
   static final int A5O_KEY_8           = 35;
   static final int A5O_KEY_9           = 36;

   static final int A5O_KEY_PAD_0       = 37;
   static final int A5O_KEY_PAD_1       = 38;
   static final int A5O_KEY_PAD_2       = 39;
   static final int A5O_KEY_PAD_3       = 40;
   static final int A5O_KEY_PAD_4       = 41;
   static final int A5O_KEY_PAD_5       = 42;
   static final int A5O_KEY_PAD_6       = 43;
   static final int A5O_KEY_PAD_7       = 44;
   static final int A5O_KEY_PAD_8       = 45;
   static final int A5O_KEY_PAD_9       = 46;

   static final int A5O_KEY_F1          = 47;
   static final int A5O_KEY_F2          = 48;
   static final int A5O_KEY_F3          = 49;
   static final int A5O_KEY_F4          = 50;
   static final int A5O_KEY_F5          = 51;
   static final int A5O_KEY_F6          = 52;
   static final int A5O_KEY_F7          = 53;
   static final int A5O_KEY_F8          = 54;
   static final int A5O_KEY_F9          = 55;
   static final int A5O_KEY_F10         = 56;
   static final int A5O_KEY_F11         = 57;
   static final int A5O_KEY_F12         = 58;

   static final int A5O_KEY_ESCAPE      = 59;
   static final int A5O_KEY_TILDE       = 60;
   static final int A5O_KEY_MINUS       = 61;
   static final int A5O_KEY_EQUALS      = 62;
   static final int A5O_KEY_BACKSPACE   = 63;
   static final int A5O_KEY_TAB         = 64;
   static final int A5O_KEY_OPENBRACE   = 65;
   static final int A5O_KEY_CLOSEBRACE  = 66;
   static final int A5O_KEY_ENTER       = 67;
   static final int A5O_KEY_SEMICOLON   = 68;
   static final int A5O_KEY_QUOTE       = 69;
   static final int A5O_KEY_BACKSLASH   = 70;
   static final int A5O_KEY_BACKSLASH2  = 71; /* DirectInput calls this DIK_OEM_102: "< > | on UK/Germany keyboards" */
   static final int A5O_KEY_COMMA       = 72;
   static final int A5O_KEY_FULLSTOP    = 73;
   static final int A5O_KEY_SLASH       = 74;
   static final int A5O_KEY_SPACE       = 75;

   static final int A5O_KEY_INSERT      = 76;
   static final int A5O_KEY_DELETE      = 77;
   static final int A5O_KEY_HOME        = 78;
   static final int A5O_KEY_END         = 79;
   static final int A5O_KEY_PGUP        = 80;
   static final int A5O_KEY_PGDN        = 81;
   static final int A5O_KEY_LEFT        = 82;
   static final int A5O_KEY_RIGHT       = 83;
   static final int A5O_KEY_UP          = 84;
   static final int A5O_KEY_DOWN        = 85;

   static final int A5O_KEY_PAD_SLASH   = 86;
   static final int A5O_KEY_PAD_ASTERISK= 87;
   static final int A5O_KEY_PAD_MINUS   = 88;
   static final int A5O_KEY_PAD_PLUS    = 89;
   static final int A5O_KEY_PAD_DELETE  = 90;
   static final int A5O_KEY_PAD_ENTER   = 91;

   static final int A5O_KEY_PRINTSCREEN = 92;
   static final int A5O_KEY_PAUSE       = 93;

   static final int A5O_KEY_ABNT_C1     = 94;
   static final int A5O_KEY_YEN         = 95;
   static final int A5O_KEY_KANA        = 96;
   static final int A5O_KEY_CONVERT     = 97;
   static final int A5O_KEY_NOCONVERT   = 98;
   static final int A5O_KEY_AT          = 99;
   static final int A5O_KEY_CIRCUMFLEX  = 100;
   static final int A5O_KEY_COLON2      = 101;
   static final int A5O_KEY_KANJI       = 102;

   static final int A5O_KEY_PAD_EQUALS  = 103;   /* MacOS X */
   static final int A5O_KEY_BACKQUOTE   = 104;   /* MacOS X */
   static final int A5O_KEY_SEMICOLON2  = 105;   /* MacOS X -- TODO: ask lillo what this should be */
   static final int A5O_KEY_COMMAND     = 106;   /* MacOS X */
   static final int A5O_KEY_BACK        = 107;
   static final int A5O_KEY_VOLUME_UP   = 108;
   static final int A5O_KEY_VOLUME_DOWN = 109;

   /* Some more standard Android keys.
    * These happen to be the ones used by the Xperia Play.
    */
   static final int A5O_KEY_SEARCH      = 110;
   static final int A5O_KEY_DPAD_CENTER = 111;
   static final int A5O_KEY_BUTTON_X    = 112;
   static final int A5O_KEY_BUTTON_Y    = 113;
   static final int A5O_KEY_DPAD_UP     = 114;
   static final int A5O_KEY_DPAD_DOWN   = 115;
   static final int A5O_KEY_DPAD_LEFT   = 116;
   static final int A5O_KEY_DPAD_RIGHT  = 117;
   static final int A5O_KEY_SELECT      = 118;
   static final int A5O_KEY_START       = 119;
   static final int A5O_KEY_BUTTON_L1   = 120;
   static final int A5O_KEY_BUTTON_R1   = 121;
   static final int A5O_KEY_BUTTON_L2   = 122;
   static final int A5O_KEY_BUTTON_R2   = 123;
   static final int A5O_KEY_BUTTON_A    = 124;
   static final int A5O_KEY_BUTTON_B    = 125;
   static final int A5O_KEY_THUMBL      = 126;
   static final int A5O_KEY_THUMBR      = 127;

   static final int A5O_KEY_UNKNOWN     = 128;

   /* All codes up to before A5O_KEY_MODIFIERS can be freely
    * assigned as additional unknown keys, like various multimedia
    * and application keys keyboards may have.
    */

   static final int A5O_KEY_MODIFIERS   = 215;

   static final int A5O_KEY_LSHIFT	    = 215;
   static final int A5O_KEY_RSHIFT      = 216;
   static final int A5O_KEY_LCTRL       = 217;
   static final int A5O_KEY_RCTRL       = 218;
   static final int A5O_KEY_ALT         = 219;
   static final int A5O_KEY_ALTGR       = 220;
   static final int A5O_KEY_LWIN        = 221;
   static final int A5O_KEY_RWIN        = 222;
   static final int A5O_KEY_MENU        = 223;
   static final int A5O_KEY_SCROLLLOCK  = 224;
   static final int A5O_KEY_NUMLOCK     = 225;
   static final int A5O_KEY_CAPSLOCK    = 226;

   static final int A5O_KEY_MAX	    = 227;

   protected static int[] keyMap = {
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_UNKNOWN
      A5O_KEY_LEFT,        // KeyEvent.KEYCODE_SOFT_LEFT
      A5O_KEY_RIGHT,       // KeyEvent.KEYCODE_SOFT_RIGHT
      A5O_KEY_HOME,        // KeyEvent.KEYCODE_HOME
      A5O_KEY_BACK,        // KeyEvent.KEYCODE_BACK
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CALL
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENDCALL
      A5O_KEY_0,           // KeyEvent.KEYCODE_0
      A5O_KEY_1,           // KeyEvent.KEYCODE_1
      A5O_KEY_2,           // KeyEvent.KEYCODE_2
      A5O_KEY_3,           // KeyEvent.KEYCODE_3
      A5O_KEY_4,           // KeyEvent.KEYCODE_4
      A5O_KEY_5,           // KeyEvent.KEYCODE_5 
      A5O_KEY_6,           // KeyEvent.KEYCODE_6 
      A5O_KEY_7,           // KeyEvent.KEYCODE_7 
      A5O_KEY_8,           // KeyEvent.KEYCODE_8 
      A5O_KEY_9,           // KeyEvent.KEYCODE_9 
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_STAR
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POUND
      A5O_KEY_UP,          // KeyEvent.KEYCODE_DPAD_UP
      A5O_KEY_DOWN,        // KeyEvent.KEYCODE_DPAD_DOWN
      A5O_KEY_LEFT,        // KeyEvent.KEYCODE_DPAD_LEFT
      A5O_KEY_RIGHT,       // KeyEvent.KEYCODE_DPAD_RIGHT
      A5O_KEY_ENTER,       // KeyEvent.KEYCODE_DPAD_CENTER
      A5O_KEY_VOLUME_UP,   // KeyEvent.KEYCODE_VOLUME_UP
      A5O_KEY_VOLUME_DOWN, // KeyEvent.KEYCODE_VOLUME_DOWN
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POWER
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CAMERA
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CLEAR
      A5O_KEY_A,           // KeyEvent.KEYCODE_A
      A5O_KEY_B,           // KeyEvent.KEYCODE_B
      A5O_KEY_C,           // KeyEvent.KEYCODE_B
      A5O_KEY_D,           // KeyEvent.KEYCODE_D
      A5O_KEY_E,           // KeyEvent.KEYCODE_E
      A5O_KEY_F,           // KeyEvent.KEYCODE_F
      A5O_KEY_G,           // KeyEvent.KEYCODE_G
      A5O_KEY_H,           // KeyEvent.KEYCODE_H
      A5O_KEY_I,           // KeyEvent.KEYCODE_I
      A5O_KEY_J,           // KeyEvent.KEYCODE_J
      A5O_KEY_K,           // KeyEvent.KEYCODE_K 
      A5O_KEY_L,           // KeyEvent.KEYCODE_L
      A5O_KEY_M,           // KeyEvent.KEYCODE_M
      A5O_KEY_N,           // KeyEvent.KEYCODE_N
      A5O_KEY_O,           // KeyEvent.KEYCODE_O
      A5O_KEY_P,           // KeyEvent.KEYCODE_P
      A5O_KEY_Q,           // KeyEvent.KEYCODE_Q
      A5O_KEY_R,           // KeyEvent.KEYCODE_R
      A5O_KEY_S,           // KeyEvent.KEYCODE_S
      A5O_KEY_T,           // KeyEvent.KEYCODE_T
      A5O_KEY_U,           // KeyEvent.KEYCODE_U
      A5O_KEY_V,           // KeyEvent.KEYCODE_V
      A5O_KEY_W,           // KeyEvent.KEYCODE_W
      A5O_KEY_X,           // KeyEvent.KEYCODE_X
      A5O_KEY_Y,           // KeyEvent.KEYCODE_Y
      A5O_KEY_Z,           // KeyEvent.KEYCODE_Z
      A5O_KEY_COMMA,       // KeyEvent.KEYCODE_COMMA 
      A5O_KEY_FULLSTOP,    // KeyEvent.KEYCODE_PERIOD 
      A5O_KEY_ALT,         // KeyEvent.KEYCODE_ALT_LEFT
      A5O_KEY_ALTGR,       // KeyEvent.KEYCODE_ALT_RIGHT
      A5O_KEY_LSHIFT,      // KeyEvent.KEYCODE_SHIFT_LEFT
      A5O_KEY_RSHIFT,      // KeyEvent.KEYCODE_SHIFT_RIGHT
      A5O_KEY_TAB,         // KeyEvent.KEYCODE_TAB
      A5O_KEY_SPACE,       // KeyEvent.KEYCODE_SPACE
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SYM
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_EXPLORER
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENVELOPE
      A5O_KEY_ENTER,       // KeyEvent.KEYCODE_ENTER
      A5O_KEY_BACKSPACE,   // KeyEvent.KEYCODE_DEL
      A5O_KEY_TILDE,       // KeyEvent.KEYCODE_GRAVE
      A5O_KEY_MINUS,       // KeyEvent.KEYCODE_MINUS
      A5O_KEY_EQUALS,      // KeyEvent.KEYCODE_EQUALS
      A5O_KEY_OPENBRACE,   // KeyEvent.KEYCODE_LEFT_BRACKET
      A5O_KEY_CLOSEBRACE,  // KeyEvent.KEYCODE_RIGHT_BRACKET
      A5O_KEY_BACKSLASH,   // KeyEvent.KEYCODE_BACKSLASH
      A5O_KEY_SEMICOLON,   // KeyEvent.KEYCODE_SEMICOLON
      A5O_KEY_QUOTE,       // KeyEvent.KEYCODE_APOSTROPHY
      A5O_KEY_SLASH,       // KeyEvent.KEYCODE_SLASH
      A5O_KEY_AT,          // KeyEvent.KEYCODE_AT
      A5O_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_HEADSETHOOK
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_FOCUS
      A5O_KEY_PAD_PLUS,    // KeyEvent.KEYCODE_PLUS
      A5O_KEY_MENU,        // KeyEvent.KEYCODE_MENU
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_NOTIFICATION
      A5O_KEY_SEARCH,      // KeyEvent.KEYCODE_SEARCH
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_STOP
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_NEXT
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PREVIOUS
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_REWIND
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_FAST_FORWARD
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MUTE
      A5O_KEY_PGUP,        // KeyEvent.KEYCODE_PAGE_UP
      A5O_KEY_PGDN,        // KeyEvent.KEYCODE_PAGE_DOWN
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_PICTSYMBOLS
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SWITCH_CHARSET
      A5O_KEY_BUTTON_A,    // KeyEvent.KEYCODE_BUTTON_A
      A5O_KEY_BUTTON_B,    // KeyEvent.KEYCODE_BUTTON_B
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_C
      A5O_KEY_BUTTON_X,    // KeyEvent.KEYCODE_BUTTON_X
      A5O_KEY_BUTTON_Y,    // KeyEvent.KEYCODE_BUTTON_Y
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_Z
      A5O_KEY_BUTTON_L1,   // KeyEvent.KEYCODE_BUTTON_L1
      A5O_KEY_BUTTON_R1,   // KeyEvent.KEYCODE_BUTTON_R1
      A5O_KEY_BUTTON_L2,   // KeyEvent.KEYCODE_BUTTON_L2
      A5O_KEY_BUTTON_R2,   // KeyEvent.KEYCODE_BUTTON_R2
      A5O_KEY_THUMBL,      // KeyEvent.KEYCODE_BUTTON_THUMBL
      A5O_KEY_THUMBR,      // KeyEvent.KEYCODE_BUTTON_THUMBR
      A5O_KEY_START,       // KeyEvent.KEYCODE_BUTTON_START
      A5O_KEY_SELECT,      // KeyEvent.KEYCODE_BUTTON_SELECT
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_MODE
      A5O_KEY_ESCAPE,      // KeyEvent.KEYCODE_ESCAPE (111)
      A5O_KEY_DELETE,      // KeyEvent.KEYCODE_FORWARD_DELETE (112)
      A5O_KEY_LCTRL,       // KeyEvent.KEYCODE_CTRL_LEFT (113)
      A5O_KEY_RCTRL,       // KeyEvent.KEYCODE_CONTROL_RIGHT (114)
      A5O_KEY_CAPSLOCK,    // KeyEvent.KEYCODE_CAPS_LOCK (115)
      A5O_KEY_SCROLLLOCK,  // KeyEvent.KEYCODE_SCROLL_LOCK (116)
      A5O_KEY_LWIN,        // KeyEvent.KEYCODE_META_LEFT (117)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (118)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (119)
      A5O_KEY_PRINTSCREEN, // KeyEvent.KEYCODE_SYSRQ (120)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (121)
      A5O_KEY_HOME,        // KeyEvent.KEYCODE_MOVE_HOME (122)
      A5O_KEY_END,         // KeyEvent.KEYCODE_MOVE_END (123)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (124)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (125)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (126)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (127)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (128)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (129)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (130)
      A5O_KEY_F1,          // KeyEvent.KEYCODE_ (131)
      A5O_KEY_F2,          // KeyEvent.KEYCODE_ (132)
      A5O_KEY_F3,          // KeyEvent.KEYCODE_ (133)
      A5O_KEY_F4,          // KeyEvent.KEYCODE_ (134)
      A5O_KEY_F5,          // KeyEvent.KEYCODE_ (135)
      A5O_KEY_F6,          // KeyEvent.KEYCODE_ (136)
      A5O_KEY_F7,          // KeyEvent.KEYCODE_ (137)
      A5O_KEY_F8,          // KeyEvent.KEYCODE_ (138)
      A5O_KEY_F9,          // KeyEvent.KEYCODE_ (139)
      A5O_KEY_F10,         // KeyEvent.KEYCODE_ (140)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (141)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (142)
      A5O_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM_LOCK (143)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (144)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (145)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (146)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (147)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (148)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (149)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (150)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (151)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (152)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (153)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (154)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (155)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (156)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (157)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (158)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (159)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (160)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (161)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (162)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (163)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (164)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (165)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (166)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (167)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (168)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (169)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (170)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (171)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (172)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (173)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (174)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (175)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (176)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (177)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (178)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (179)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (180)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (181)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (182)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (183)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (184)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (185)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (186)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (187)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (188)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (189)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (190)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (191)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (192)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (193)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (194)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (195)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (196)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (197)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (198)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (199)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (200)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (201)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (202)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (203)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (204)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (205)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (206)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (207)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (208)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (209)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (210)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (211)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (212)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (213)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (214)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (215)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (216)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (217)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (218)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (219)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (220)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (221)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (222)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (223)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (224)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (225)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (226)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (227)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (228)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (229)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (230)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (231)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (232)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (233)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (234)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (235)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (236)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (237)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (238)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (239)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (240)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (241)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (242)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (243)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (244)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (245)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (246)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (247)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (248)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (249)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (250)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (251)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (252)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (253)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (254)
      A5O_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (255)
   };

   /* Return Allegro key code for Android key code. */
   static int alKey(int keyCode) {
      if (keyCode < keyMap.length)
         return keyMap[keyCode];
      else
         return A5O_KEY_UNKNOWN;
   }
}

/* vim: set sts=3 sw=3 et: */
