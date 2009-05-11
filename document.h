/* $Id$ */

#ifndef UNFEED_DOCUMENT_H
#define UNFEED_DOCUMENT_H

#include <stdlib.h>
#include <sys/queue.h>

struct feed {
	char	*title;
	SLIST_HEAD(, item)	items;
	SLIST_ENTRY(feed)	next;
};

#define INIT_FEED(_d) do {						\
		(_d)->title = NULL;					\
		SLIST_INIT(&(_d)->items);				\
	} while(0)

struct item {
	char	*title;
	char	*link;
	char	*descr;
	char	*author;
	char	*id;
	char	*date;
	time_t	 time;
	SLIST_HEAD(, category)	categories;
	SLIST_HEAD(, enclosure)	enclosures;
	int			flags;
	SLIST_ENTRY(item)	next;
};

#define INIT_ITEM(_a) do {						\
		(_a)->title = (_a)->link = (_a)->descr = (_a)->author	\
		    = (_a)->id = (_a)->date = NULL;			\
		(_a)->time = (time_t)-1;				\
		SLIST_INIT(&(_a)->categories);				\
		SLIST_INIT(&(_a)->enclosures);				\
		(_a)->flags = 0;					\
	} while(0)

enum ITEM_FLAGS {
	ITEM_OLD	= 0x0001,
};

struct category {
	char	*name;
	SLIST_ENTRY(category)	next;
};

struct enclosure {
	char	*url;
	char	*size;
	char	*type;
	SLIST_ENTRY(enclosure)	next;
};

#define INIT_ENCLOSURE(_e) do {						\
		(_e)->url = (_e)->size = (_e)->type = NULL;		\
	} while(0)

#endif
