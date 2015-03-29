#include <pebble.h>

#include "feed_item_window.h"

#define TEXT_PADDING 3
#define CORNER_RADIUS 4

#define ICON_TO_TEXT_PADDING 6

#define IMAGE_MESSAGE_ICON_WIDTH 15
#define IMAGE_MESSAGE_ICON_HEIGHT 8
  
#define MAX_DATE_HEIGHT 200
#define MAX_TITLE_HEIGHT 200
#define MAX_BODY_HEIGHT 2000

static Window *feed_item_window;
static GBitmap *message_icon_image;

static Layer *feed_item_content_layer;

static Layer *feed_item_date_layer;
static TextLayer *feed_item_date_text_layer;

static TextLayer *feed_item_title_text_layer;
static TextLayer *feed_item_body_text_layer;

static ScrollLayer *feed_item_scroll_layer;

static char feed_item_date[32];
static char feed_item_title[128];
static char feed_item_body[2048];

static void feed_item_content_layer_draw(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, CORNER_RADIUS, GCornersAll);
}

static void feed_item_date_layer_update_draw(Layer *layer, GContext* ctx) {
    graphics_draw_bitmap_in_rect(ctx, message_icon_image, GRect(0, 8, IMAGE_MESSAGE_ICON_WIDTH, IMAGE_MESSAGE_ICON_HEIGHT));
}

static void feed_item_window_load(Window* window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect window_bounds = layer_get_frame(window_layer);    
    
    // Create date TextLayer
    GRect date_text_layer_bounds = GRect(IMAGE_MESSAGE_ICON_WIDTH + ICON_TO_TEXT_PADDING, 0, window_bounds.size.w, MAX_DATE_HEIGHT);
    feed_item_date_text_layer = text_layer_create(date_text_layer_bounds);
    
    text_layer_set_background_color(feed_item_date_text_layer, GColorClear);
    text_layer_set_font(feed_item_date_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_overflow_mode(feed_item_date_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text(feed_item_date_text_layer, feed_item_date);  
    text_layer_set_text_color(feed_item_date_text_layer, GColorBlack);
    
    GSize date_text_size = text_layer_get_content_size(feed_item_date_text_layer);
    date_text_size.h = date_text_size.h + 3; // Additional padding to account for letters with "tails", e.g. g
    text_layer_set_size(feed_item_date_text_layer, date_text_size);

    // Figure out the total width including both the text and icon
    int feed_item_date_layer_width = IMAGE_MESSAGE_ICON_WIDTH + ICON_TO_TEXT_PADDING + date_text_size.w;
    feed_item_date_layer = layer_create(GRect((window_bounds.size.w - feed_item_date_layer_width) / 2, 0, feed_item_date_layer_width, date_text_size.h));
    layer_set_update_proc(feed_item_date_layer, feed_item_date_layer_update_draw);
    
    layer_add_child(feed_item_date_layer, text_layer_get_layer(feed_item_date_text_layer));
    
    // Create title TextLayer
    GRect title_text_layer_bounds = GRect(TEXT_PADDING, date_text_size.h, window_bounds.size.w - (2 * TEXT_PADDING), MAX_TITLE_HEIGHT);
    feed_item_title_text_layer = text_layer_create(title_text_layer_bounds);
    
    text_layer_set_background_color(feed_item_title_text_layer, GColorClear);
    text_layer_set_font(feed_item_title_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_overflow_mode(feed_item_title_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text(feed_item_title_text_layer, feed_item_title);  
    text_layer_set_text_color(feed_item_title_text_layer, GColorBlack);

    GSize title_text_size = text_layer_get_content_size(feed_item_title_text_layer);
    title_text_size.h = title_text_size.h + 4; // Additional padding to account for letters with "tails", e.g. g
    text_layer_set_size(feed_item_title_text_layer, title_text_size);

    // Create body TextLayer
    GRect body_text_layer_bounds = GRect(TEXT_PADDING, date_text_size.h + title_text_size.h + 4, window_bounds.size.w - (2 * TEXT_PADDING), MAX_BODY_HEIGHT);
    feed_item_body_text_layer = text_layer_create(body_text_layer_bounds);
    
    text_layer_set_background_color(feed_item_body_text_layer, GColorClear);
    text_layer_set_font(feed_item_body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_overflow_mode(feed_item_body_text_layer, GTextOverflowModeWordWrap);
    text_layer_set_text(feed_item_body_text_layer, feed_item_body);  
    text_layer_set_text_color(feed_item_body_text_layer, GColorBlack);

    GSize body_text_size = text_layer_get_content_size(feed_item_body_text_layer);
    body_text_size.h = body_text_size.h + 4; // Additional padding to account for letters with "tails", e.g. g
    text_layer_set_size(feed_item_body_text_layer, body_text_size);
    
    // Create a 'dummy' layer to hold everything, this layer will have the rounded corners drawn on
    feed_item_content_layer = layer_create(GRect(0, 0, window_bounds.size.w, date_text_size.h + title_text_size.h + body_text_size.h + 10));
    layer_add_child(feed_item_content_layer, feed_item_date_layer);
    layer_add_child(feed_item_content_layer, text_layer_get_layer(feed_item_title_text_layer));
    layer_add_child(feed_item_content_layer, text_layer_get_layer(feed_item_body_text_layer));
    layer_set_update_proc(feed_item_content_layer, feed_item_content_layer_draw);
    
    // Create the ScrollLayer
    feed_item_scroll_layer = scroll_layer_create(window_bounds);
    scroll_layer_add_child(feed_item_scroll_layer, feed_item_content_layer);
    scroll_layer_set_content_size(feed_item_scroll_layer, layer_get_bounds(feed_item_content_layer).size);
    scroll_layer_set_click_config_onto_window(feed_item_scroll_layer, window);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(feed_item_scroll_layer));
}

static void feed_item_window_unload(Window* window) {
    scroll_layer_destroy(feed_item_scroll_layer);
    
    layer_destroy(feed_item_content_layer);
    layer_destroy(feed_item_date_layer);
    
    text_layer_destroy(feed_item_body_text_layer);
    text_layer_destroy(feed_item_title_text_layer);
    text_layer_destroy(feed_item_date_text_layer);
}

void feed_item_window_create(void) {
    feed_item_window = window_create();
    window_set_background_color(feed_item_window, GColorBlack);

    window_set_window_handlers(feed_item_window, (WindowHandlers) {
        .load = feed_item_window_load,
        .unload = feed_item_window_unload,
    });
    
    message_icon_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUD_ICON_15x8);
}

void feed_item_window_destroy(void) {
    gbitmap_destroy(message_icon_image);
    window_destroy(feed_item_window);
}

void feed_item_window_show(const FeedItem *feed_item) {
    snprintf(feed_item_date, sizeof(feed_item_date), "%s", feed_item->relative_date);
    snprintf(feed_item_title, sizeof(feed_item_title), "%s", feed_item->title);
    snprintf(feed_item_body, sizeof(feed_item_body), "%s", feed_item->body);
    
    window_stack_push(feed_item_window, true);
}

void feed_item_window_hide(void) {
    window_stack_remove(feed_item_window, false);
}