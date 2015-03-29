#ifndef _MESSAGE_WINDOW_H
#define _MESSAGE_WINDOW_H

void message_window_create(void);
void message_window_destroy(void);
void message_window_show(const char *message);
void message_window_hide(void);

#endif // _MESSAGE_WINDOW_H