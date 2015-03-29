#include <pebble.h>

#include "app_message.h"
#include "message_window.h"
#include "feed_menu_window.h"
#include "feed_item_window.h"
    
#define CONNECTING_MESSAGE "Connecting to Salesforce..."

void init(void) {
    // Create all the windows
    message_window_create();
    feed_menu_window_create();
    feed_item_window_create();

    // Open AppMessage
    app_message_init();
    
    message_window_show(CONNECTING_MESSAGE);
    send_watch_app_opened_message(); 
}

void deinit(void) {
    feed_item_window_destroy();
    feed_menu_window_destroy();
    message_window_destroy();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}