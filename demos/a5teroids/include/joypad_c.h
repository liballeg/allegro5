#ifdef __cplusplus
extern "C" {
#endif

void joypad_start(void);
void joypad_find(void);
void joypad_stop_finding(void);
void get_joypad_state(bool *u, bool *d, bool *l, bool *r, bool *b, bool *esc);
bool is_joypad_connected(void);

#ifdef __cplusplus
}
#endif
