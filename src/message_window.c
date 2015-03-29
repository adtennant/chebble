#include <pebble.h>

#include "message_window.h"

static Window *message_window;
static TextLayer *message_text_layer;

static char message_text[128];

static void message_window_load(Window* window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_frame(window_layer);
    
    message_text_layer = text_layer_create(window_bounds);
    text_layer_set_background_color(message_text_layer, GColorBlack);
    text_layer_set_font(message_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_overflow_mode(message_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(message_text_layer, GTextAlignmentCenter);
    text_layer_set_text_color(message_text_layer, GColorWhite);
    text_layer_set_text(message_text_layer, message_text);
                              
    GSize message_text_size = text_layer_get_content_size(message_text_layer);
    layer_set_frame(text_layer_get_layer(message_text_layer), GRect(0, (window_bounds.size.h - message_text_size.h) / 2, window_bounds.size.w, window_bounds.size.h));

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(message_text_layer));
}

static void message_window_unload(Window* window) {
    text_layer_destroy(message_text_layer);
}

void message_window_create(void) {
    message_window = window_create();
    window_set_background_color(message_window, GColorBlack);

    window_set_window_handlers(message_window, (WindowHandlers) {
        .load = message_window_load,
        .unload = message_window_unload,
    });
}

void message_window_destroy(void) {
    window_destroy(message_window);
}

void message_window_show(const char *message) {
    snprintf(message_text, sizeof(message_text), "%s", message);
    
    window_stack_push(message_window, true);
}

void message_window_hide(void) {
    window_stack_remove(message_window, false);
}