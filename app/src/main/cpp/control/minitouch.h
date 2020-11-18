#ifndef MINITOUCH_H
#define MINITOUCH_H

int minitouch_init();
void minitouch_uninit();
int touch_down(int contact, int x, int y, int pressure);
int touch_move(int contact, int x, int y, int pressure);
int touch_up(int contact);
int touch_panic_reset_all();
void minitouch_get_max_xy(int *max_x, int *max_y);
int minitouch_commit();

#endif