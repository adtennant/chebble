#ifndef _FEED_ITEM_H
#define _FEED_ITEM_H

struct FeedItem {
    const char *title;
    const char *subtitle;
    const char *body;
    const char *relative_date;
    int section_index;
};

typedef struct FeedItem FeedItem;

#endif // _FEED_ITEM_H