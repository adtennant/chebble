#ifndef _FEED_ITEM_WINDOW_H
#define _FEED_ITEM_WINDOW_H

#include "feed_item.h"

void feed_item_window_create(void);
void feed_item_window_destroy(void);
void feed_item_window_show(const FeedItem *feed_item);
void feed_item_window_hide(void);

#endif // _FEED_ITEM_WINDOW_H