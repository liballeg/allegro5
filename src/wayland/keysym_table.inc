static struct {
   KeySym keysym;
   int allegro_key;
}
translation_table[] = {
   {XKB_KEY_a, ALLEGRO_KEY_A},
   {XKB_KEY_b, ALLEGRO_KEY_B},
   {XKB_KEY_c, ALLEGRO_KEY_C},
   {XKB_KEY_d, ALLEGRO_KEY_D},
   {XKB_KEY_e, ALLEGRO_KEY_E},
   {XKB_KEY_f, ALLEGRO_KEY_F},
   {XKB_KEY_g, ALLEGRO_KEY_G},
   {XKB_KEY_h, ALLEGRO_KEY_H},
   {XKB_KEY_i, ALLEGRO_KEY_I},
   {XKB_KEY_j, ALLEGRO_KEY_J},
   {XKB_KEY_k, ALLEGRO_KEY_K},
   {XKB_KEY_l, ALLEGRO_KEY_L},
   {XKB_KEY_m, ALLEGRO_KEY_M},
   {XKB_KEY_n, ALLEGRO_KEY_N},
   {XKB_KEY_o, ALLEGRO_KEY_O},
   {XKB_KEY_p, ALLEGRO_KEY_P},
   {XKB_KEY_q, ALLEGRO_KEY_Q},
   {XKB_KEY_r, ALLEGRO_KEY_R},
   {XKB_KEY_s, ALLEGRO_KEY_S},
   {XKB_KEY_t, ALLEGRO_KEY_T},
   {XKB_KEY_u, ALLEGRO_KEY_U},
   {XKB_KEY_v, ALLEGRO_KEY_V},
   {XKB_KEY_w, ALLEGRO_KEY_W},
   {XKB_KEY_x, ALLEGRO_KEY_X},
   {XKB_KEY_y, ALLEGRO_KEY_Y},
   {XKB_KEY_z, ALLEGRO_KEY_Z},
   {XKB_KEY_0, ALLEGRO_KEY_0},
   {XKB_KEY_1, ALLEGRO_KEY_1},
   {XKB_KEY_2, ALLEGRO_KEY_2},
   {XKB_KEY_3, ALLEGRO_KEY_3},
   {XKB_KEY_4, ALLEGRO_KEY_4},
   {XKB_KEY_5, ALLEGRO_KEY_5},
   {XKB_KEY_6, ALLEGRO_KEY_6},
   {XKB_KEY_7, ALLEGRO_KEY_7},
   {XKB_KEY_8, ALLEGRO_KEY_8},
   {XKB_KEY_9, ALLEGRO_KEY_9},

   /* Double mappings for numeric keyboard.
    * If an X server actually uses both at the same time, Allegro will
    * detect them as the same. But normally, an X server just reports it as
    * either of them, and therefore we always get the keys as ALLEGRO_KEY_PAD_*.
    */
   {XKB_KEY_KP_0, ALLEGRO_KEY_PAD_0},
   {XKB_KEY_KP_Insert, ALLEGRO_KEY_PAD_0},
   {XKB_KEY_KP_1, ALLEGRO_KEY_PAD_1},
   {XKB_KEY_KP_End, ALLEGRO_KEY_PAD_1},
   {XKB_KEY_KP_2, ALLEGRO_KEY_PAD_2},
   {XKB_KEY_KP_Down, ALLEGRO_KEY_PAD_2},
   {XKB_KEY_KP_3, ALLEGRO_KEY_PAD_3},
   {XKB_KEY_KP_Page_Down, ALLEGRO_KEY_PAD_3},
   {XKB_KEY_KP_4, ALLEGRO_KEY_PAD_4},
   {XKB_KEY_KP_Left, ALLEGRO_KEY_PAD_4},
   {XKB_KEY_KP_5, ALLEGRO_KEY_PAD_5},
   {XKB_KEY_KP_Begin, ALLEGRO_KEY_PAD_5},
   {XKB_KEY_KP_6, ALLEGRO_KEY_PAD_6},
   {XKB_KEY_KP_Right, ALLEGRO_KEY_PAD_6},
   {XKB_KEY_KP_7, ALLEGRO_KEY_PAD_7},
   {XKB_KEY_KP_Home, ALLEGRO_KEY_PAD_7},
   {XKB_KEY_KP_8, ALLEGRO_KEY_PAD_8},
   {XKB_KEY_KP_Up, ALLEGRO_KEY_PAD_8},
   {XKB_KEY_KP_9, ALLEGRO_KEY_PAD_9},
   {XKB_KEY_KP_Page_Up, ALLEGRO_KEY_PAD_9},
   {XKB_KEY_KP_Delete, ALLEGRO_KEY_PAD_DELETE},
   {XKB_KEY_KP_Decimal, ALLEGRO_KEY_PAD_DELETE},

   /* Double mapping!
    * Same as above - but normally, the X server just reports one or the
    * other for the Pause key, and the other is not reported for any key.
    */
   {XKB_KEY_Pause, ALLEGRO_KEY_PAUSE},
   {XKB_KEY_Break, ALLEGRO_KEY_PAUSE},

   {XKB_KEY_F1, ALLEGRO_KEY_F1},
   {XKB_KEY_F2, ALLEGRO_KEY_F2},
   {XKB_KEY_F3, ALLEGRO_KEY_F3},
   {XKB_KEY_F4, ALLEGRO_KEY_F4},
   {XKB_KEY_F5, ALLEGRO_KEY_F5},
   {XKB_KEY_F6, ALLEGRO_KEY_F6},
   {XKB_KEY_F7, ALLEGRO_KEY_F7},
   {XKB_KEY_F8, ALLEGRO_KEY_F8},
   {XKB_KEY_F9, ALLEGRO_KEY_F9},
   {XKB_KEY_F10, ALLEGRO_KEY_F10},
   {XKB_KEY_F11, ALLEGRO_KEY_F11},
   {XKB_KEY_F12, ALLEGRO_KEY_F12},
   {XKB_KEY_Escape, ALLEGRO_KEY_ESCAPE},
   {XKB_KEY_grave, ALLEGRO_KEY_TILDE}, /* US left of 1 */
   {XKB_KEY_minus, ALLEGRO_KEY_MINUS}, /* US right of 0 */
   {XKB_KEY_equal, ALLEGRO_KEY_EQUALS}, /* US 2 right of 0 */
   {XKB_KEY_BackSpace, ALLEGRO_KEY_BACKSPACE},
   {XKB_KEY_Tab, ALLEGRO_KEY_TAB},
   {XKB_KEY_bracketleft, ALLEGRO_KEY_OPENBRACE}, /* US right of P */
   {XKB_KEY_bracketright, ALLEGRO_KEY_CLOSEBRACE}, /* US 2 right of P */
   {XKB_KEY_Return, ALLEGRO_KEY_ENTER},
   {XKB_KEY_semicolon, ALLEGRO_KEY_SEMICOLON}, /* US right of L */
   {XKB_KEY_apostrophe, ALLEGRO_KEY_QUOTE}, /* US 2 right of L */
   {XKB_KEY_backslash, ALLEGRO_KEY_BACKSLASH}, /* US 3 right of L */
   {XKB_KEY_less, ALLEGRO_KEY_BACKSLASH2}, /* US left of Y */
   {XKB_KEY_comma, ALLEGRO_KEY_COMMA}, /* US right of M */
   {XKB_KEY_period, ALLEGRO_KEY_FULLSTOP}, /* US 2 right of M */
   {XKB_KEY_slash, ALLEGRO_KEY_SLASH}, /* US 3 right of M */
   {XKB_KEY_space, ALLEGRO_KEY_SPACE},
   {XKB_KEY_Insert, ALLEGRO_KEY_INSERT},
   {XKB_KEY_Delete, ALLEGRO_KEY_DELETE},
   {XKB_KEY_Home, ALLEGRO_KEY_HOME},
   {XKB_KEY_End, ALLEGRO_KEY_END},
   {XKB_KEY_Page_Up, ALLEGRO_KEY_PGUP},
   {XKB_KEY_Page_Down, ALLEGRO_KEY_PGDN},
   {XKB_KEY_Left, ALLEGRO_KEY_LEFT},
   {XKB_KEY_Right, ALLEGRO_KEY_RIGHT},
   {XKB_KEY_Up, ALLEGRO_KEY_UP},
   {XKB_KEY_Down, ALLEGRO_KEY_DOWN},
   {XKB_KEY_KP_Divide, ALLEGRO_KEY_PAD_SLASH},
   {XKB_KEY_KP_Multiply, ALLEGRO_KEY_PAD_ASTERISK},
   {XKB_KEY_KP_Subtract, ALLEGRO_KEY_PAD_MINUS},
   {XKB_KEY_KP_Add, ALLEGRO_KEY_PAD_PLUS},
   {XKB_KEY_KP_Enter, ALLEGRO_KEY_PAD_ENTER},
   {XKB_KEY_Print, ALLEGRO_KEY_PRINTSCREEN},

   //{, ALLEGRO_KEY_ABNT_C1},
   //{, ALLEGRO_KEY_YEN},
   //{, ALLEGRO_KEY_KANA},
   //{, ALLEGRO_KEY_CONVERT},
   //{, ALLEGRO_KEY_NOCONVERT},
   //{, ALLEGRO_KEY_AT},
   //{, ALLEGRO_KEY_CIRCUMFLEX},
   //{, ALLEGRO_KEY_COLON2},
   //{, ALLEGRO_KEY_KANJI},
   {XKB_KEY_KP_Equal, ALLEGRO_KEY_PAD_EQUALS},  /* MacOS X */
   //{, ALLEGRO_KEY_BACKQUOTE},  /* MacOS X */
   //{, ALLEGRO_KEY_SEMICOLON},  /* MacOS X */
   //{, ALLEGRO_KEY_COMMAND},  /* MacOS X */

   {XKB_KEY_Shift_L, ALLEGRO_KEY_LSHIFT},
   {XKB_KEY_Shift_R, ALLEGRO_KEY_RSHIFT},
   {XKB_KEY_Control_L, ALLEGRO_KEY_LCTRL},
   {XKB_KEY_Control_R, ALLEGRO_KEY_RCTRL},
   {XKB_KEY_Alt_L, ALLEGRO_KEY_ALT},

   /* Double mappings. This is a bit of a problem, since you can configure
    * X11 differently to what report for those keys.
    */
   {XKB_KEY_Alt_R, ALLEGRO_KEY_ALTGR},
   {XKB_KEY_ISO_Level3_Shift, ALLEGRO_KEY_ALTGR},
   {XKB_KEY_Meta_L, ALLEGRO_KEY_LWIN},
   {XKB_KEY_Super_L, ALLEGRO_KEY_LWIN},
   {XKB_KEY_Meta_R, ALLEGRO_KEY_RWIN},
   {XKB_KEY_Super_R, ALLEGRO_KEY_RWIN},

   {XKB_KEY_Menu, ALLEGRO_KEY_MENU},
   {XKB_KEY_Scroll_Lock, ALLEGRO_KEY_SCROLLLOCK},
   {XKB_KEY_Num_Lock, ALLEGRO_KEY_NUMLOCK},
   {XKB_KEY_Caps_Lock, ALLEGRO_KEY_CAPSLOCK}
};
