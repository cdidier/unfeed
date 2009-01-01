/* $Id$ */

#ifndef UNFEED_DOCUMENT_H
#define UNFEED_DOCUMENT_H

#include <stdlib.h>
#include <sys/queue.h>

struct document {
	char	*title;
	SLIST_HEAD(, article)	articles;
	SLIST_ENTRY(document)	next;
};

#define INIT_DOCUMENT(_d) do {						\
		(_d)->title = NULL;					\
		SLIST_INIT(&(_d)->articles);				\
	} while(/* CONSTCOND */ 0)

struct article {
	char	*title;
	char	*link;
	char	*descr;
	char	*author;
	char	*id;
	char	*date;
	SLIST_HEAD(, category)	categories;
	SLIST_HEAD(, enclosure)	enclosures;
	SLIST_ENTRY(article)	next;
};

#define INIT_ARTICLE(_a) do {						\
		(_a)->title = (_a)->link = (_a)->descr = (_a)->author	\
		    = (_a)->id = (_a)->date = NULL;			\
		SLIST_INIT(&(_a)->categories);				\
		SLIST_INIT(&(_a)->enclosures);				\
	} while(/* CONSTCOND */ 0)

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

#endif
