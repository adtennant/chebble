#ifndef _APP_MESSAGE_H
#define _APP_MESSAGE_H

void app_message_init(void);

void send_watch_app_opened_message(void);
void send_refresh_feed_message(void);
void send_feed_item_selected_message(const int index);

#endif // _APP_MESSAGE_H