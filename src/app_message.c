#include <pebble.h>

#include "app_message.h"
#include "feed_menu_window.h"
#include "feed_item_window.h"
#include "message_keys.h"
#include "message_types.h"
#include "message_window.h"

// Error messages
#define NO_CREDENTIALS_ERROR_MESSAGE "No credentials saved."
#define GET_FEED_ERROR_MESSAGE "Error retrieving Chatter feed."
#define REFRESH_TOKEN_ERROR_MESSAGE "Error refreshing access token."
#define UNABLE_TO_CONNECT_ERROR_MESSAGE "Unable to connect to watch."

#define NO_TYPE_ERROR_MESSAGE "Message receieved with no type."
#define TYPE_NOT_RECOGNIZED_ERROR_MESSAGE "Message type %d not recognized!"

// AppMessage lifecycle messages
#define INBOX_RECEIVED_MESSAGE "Message receieved!"
#define INBOX_DROPPED_MESSAGE "Message dropped! Reason: %s"
#define OUTBOX_SUCCESS_MESSAGE "Outbox send success!"
#define OUTBOX_FAILURE_MESSAGE "Outbox send failed! Reason: %s"

#define MAX_RETRY_COUNT 5

static int retry_count = 0;

static char *translate_error(AppMessageResult result) {
    switch (result) {
        case APP_MSG_OK: return "APP_MSG_OK";
        case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
        case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
        case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
        case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
        case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
        case APP_MSG_BUSY: return "APP_MSG_BUSY";
        case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
        case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
        case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
        case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
        case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
        case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
        case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
        default: return "UNKNOWN ERROR";
    }
}

// Tap handler
static void tap_handler(AccelAxisType axis, int32_t direction) {
    send_refresh_feed_message();
}

// Message handler functions
static void no_credentials_error_message_handler(DictionaryIterator *iterator, void *context) {
    message_window_hide();
    message_window_show(NO_CREDENTIALS_ERROR_MESSAGE);
}

static void get_feed_error_message_handler(DictionaryIterator *iterator, void *context) {
    message_window_hide();
    message_window_show(GET_FEED_ERROR_MESSAGE);
}

static void refresh_token_error_message_handler(DictionaryIterator *iterator, void *context) {
    message_window_hide();
    message_window_show(REFRESH_TOKEN_ERROR_MESSAGE);
}

static void feed_item_list_message_handler(DictionaryIterator *iterator, void *context) {
    int num_sections = dict_find(iterator, NUM_SECTIONS)->value->int32;
    FeedSection sections[num_sections];
    
    for(int i = 0; i < num_sections; i++) {
        sections[i] = (FeedSection) {
            .title = dict_find(iterator, SECTION_X_TITLE(i))->value->cstring,
            .num_rows = dict_find(iterator, SECTION_X_NUM_ROWS(i))->value->int32
        };
    }
    
    int num_feed_items = dict_find(iterator, NUM_FEED_ITEMS)->value->int32;
    FeedItem feed_items[num_feed_items];
    
    for(int i = 0; i < num_feed_items; i++) {
        feed_items[i] = (FeedItem) {
            .title = dict_find(iterator, FEED_ITEM_X_TITLE(i))->value->cstring,
            .subtitle = dict_find(iterator, FEED_ITEM_X_SUBTITLE(i))->value->cstring,
            .section_index = dict_find(iterator, FEED_ITEM_X_SECTION_INDEX(i))->value->int32
        };
    }
    
    feed_item_window_hide();
    feed_menu_window_hide();
    message_window_hide();
    
    accel_tap_service_subscribe(tap_handler);
    feed_menu_window_show(sections, num_sections, feed_items, num_feed_items);
}

static void feed_item_display_message_handler(DictionaryIterator *iterator, void *context) {
    FeedItem feed_item = (FeedItem) {
        .title = dict_find(iterator, FEED_ITEM_TITLE)->value->cstring,
        .body = dict_find(iterator, FEED_ITEM_BODY)->value->cstring,
        .relative_date = dict_find(iterator, FEED_ITEM_RELATIVE_DATE)->value->cstring,
    };
    
    feed_item_window_show(&feed_item);
}

static void (*message_handlers[]) (DictionaryIterator *, void *) = {
    [NO_CREDENTIALS_ERROR] = no_credentials_error_message_handler,
    [GET_FEED_ERROR] = get_feed_error_message_handler,
    [REFRESH_TOKEN_ERROR] = refresh_token_error_message_handler,
    
    [SHOW_FEED_MENU] = feed_item_list_message_handler,
    [SHOW_FEED_ITEM] = feed_item_display_message_handler,
};

// AppMessage callbacks
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, INBOX_RECEIVED_MESSAGE);

    // Get the type of the message
    Tuple *t = dict_find(iterator, MESSAGE_TYPE_KEY);

    if(!t) {
        APP_LOG(APP_LOG_LEVEL_ERROR, NO_TYPE_ERROR_MESSAGE);
        return;
    }
    
    int type = t->value->int32;
    
    if(type < NUM_MESSAGE_HANDLERS) {
        message_handlers[type](iterator, context);
    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, TYPE_NOT_RECOGNIZED_ERROR_MESSAGE, type);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, INBOX_DROPPED_MESSAGE, translate_error(reason));
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, OUTBOX_FAILURE_MESSAGE, translate_error(reason));

    if((reason == APP_MSG_SEND_TIMEOUT || reason == APP_MSG_BUSY || reason == APP_MSG_NOT_CONNECTED) && retry_count < MAX_RETRY_COUNT) {
        switch(dict_find(iterator, MESSAGE_TYPE_KEY)->value->int32) {
            case WATCH_APP_OPENED: {
                send_watch_app_opened_message();
                break;
            }
            case FEED_ITEM_SELECTED: {
                int feed_item_index = dict_find(iterator, SELECTED_FEED_ITEM_INDEX)->value->int32;
                send_feed_item_selected_message(feed_item_index);
                break;
            }
        }
        
        retry_count++;
    }
    else {
        message_window_show(UNABLE_TO_CONNECT_ERROR_MESSAGE);
    }
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, OUTBOX_SUCCESS_MESSAGE);
    retry_count = 0;
}

void app_message_init(void) {
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void send_watch_app_opened_message(void) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_TYPE_KEY, WATCH_APP_OPENED);
    dict_write_end(iter);
    app_message_outbox_send();
}

void send_refresh_feed_message(void) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_TYPE_KEY, REFRESH_FEED);
    dict_write_end(iter);
    app_message_outbox_send();
}

void send_feed_item_selected_message(const int index) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_TYPE_KEY, FEED_ITEM_SELECTED);
    dict_write_uint8(iter, SELECTED_FEED_ITEM_INDEX, index);
    dict_write_end(iter);
    app_message_outbox_send();
}