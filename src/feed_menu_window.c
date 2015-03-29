#include <pebble.h>

#include "app_message.h"
#include "feed_menu_window.h"

static Window *feed_menu_window;
static MenuLayer *feed_menu_layer;

static char section_titles[10][32];
static char feed_item_titles[10][32];
static char feed_item_subtitles[10][32];

static int num_sections;
static int num_rows[10];

// MenuLayer callbacks
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return num_sections;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return num_rows[section_index];
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, section_titles[section_index]);
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    int row_offset = 0;

    if(cell_index->section > 0) {
        for(int i = 0; i < cell_index->section; i ++) {
            row_offset += num_rows[i];
        }
    }

    int row_index = row_offset + cell_index->row;
    menu_cell_basic_draw(ctx, cell_layer, feed_item_titles[row_index], feed_item_subtitles[row_index], NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    int row_offset = 0;

    if(cell_index->section > 0) {
        for(int i = 0; i < cell_index->section; i ++) {
            row_offset += num_rows[i];
        }
    }

    int row_index = row_offset + cell_index->row;
    send_feed_item_selected_message(row_index); 

}

static void feed_menu_window_load(Window* window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    feed_menu_layer = menu_layer_create(bounds);
    
    menu_layer_set_callbacks(feed_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_sections = menu_get_num_sections_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .get_header_height = menu_get_header_height_callback,
        .draw_header = menu_draw_header_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });

    menu_layer_set_click_config_onto_window(feed_menu_layer, window);

    layer_add_child(window_layer, menu_layer_get_layer(feed_menu_layer));
}

static void feed_menu_window_unload(Window* window) {
    menu_layer_destroy(feed_menu_layer);
}

void feed_menu_window_create(void) {
    feed_menu_window = window_create();

    window_set_window_handlers(feed_menu_window, (WindowHandlers) {
        .load = feed_menu_window_load,
        .unload = feed_menu_window_unload,
    });
}

void feed_menu_window_destroy(void) {
    window_destroy(feed_menu_window);
}

void feed_menu_window_show(const FeedSection feed_sections[], int num_feed_sections, const FeedItem feed_items[], int num_feed_items) {
    num_sections = num_feed_sections;

    for(int i = 0; i < num_feed_sections; i++) {
        snprintf(section_titles[i], sizeof(section_titles[i]), "%s", feed_sections[i].title);
        num_rows[i] = feed_sections[i].num_rows;
    }

    for(int i = 0; i < num_feed_items; i++) {
        snprintf(feed_item_titles[i], sizeof(feed_item_titles[i]), "%s", feed_items[i].title);
        snprintf(feed_item_subtitles[i], sizeof(feed_item_subtitles[i]), "%s", feed_items[i].subtitle);
    }

    window_stack_push(feed_menu_window, true);

    vibes_short_pulse();
}

void feed_menu_window_hide(void) {
    window_stack_remove(feed_menu_window, false);
}