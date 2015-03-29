#ifndef _FEED_MENU_WINDOW_H
#define _FEED_MENU_WINDOW_H

#include "feed_item.h"
#include "feed_section.h"

void feed_menu_window_create(void);
void feed_menu_window_destroy(void);
void feed_menu_window_show(const FeedSection feed_sections[], int num_feed_sections, const FeedItem feed_items[], int num_feed_items);
void feed_menu_window_hide(void);

#endif // _FEED_MENU_WINDOW_H