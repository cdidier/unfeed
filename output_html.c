/*
 * $Id$
 *
 * Copyright (c) 2008,2009 Colin Didier <cdidier@cybione.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "document.h"

#define MARKER_TAG "%%"

static char *PAGE_TEMPLATE =
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
    "<html>\n"
    "<head>\n"
    "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
    "  <title>unfeed - last update: %%PAGE_UPDATE%%</title>\n"
    "</head>\n"
    "<body>\n"
    "%%FEEDS%%\n"
    "</body>\n"
    "</html>\n";

static char *FEED_TEMPLATE =
    "<h2>%%FEED_TITLE%%</h2>\n"
    "%%ITEMS%%\n";

static char *ITEM_TEMPLATE =
    "  <h3>%%ITEM_TITLE%%</h3>\n"
    "    Date: %%ITEM_DATE%%<br>\n"
    "    Author: %%ITEM_AUTHOR%%<br>\n"
    "    Link: <a href=\"%%ITEM_LINK%%\">%%ITEM_LINK%%</a><br>\n"
    "    Category: %%ITEM_CATEGORIES%%<br>\n"
    "    <br>\n"
    "    %%ITEM_DESCR%%<br>\n";

enum {
	TYPE_PAGE,
	TYPE_FEED,
	TYPE_ITEM
};

static char *template_page;
static char *template_feed;
static char *template_item;

static void parse_template(const char *, int, void *);

static void
page_markers(const char *m, struct feed *feeds)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;

	if (strcmp(m, "PAGE_UPDATE") == 0) {
	} else if (strcmp(m, "FEEDS") == 0) {
		SLIST_FIRST(&list) = feeds;
		SLIST_FOREACH(feed, &list, next)
			parse_template(template_feed, TYPE_FEED, feed);
	}
}

static void
feed_markers(const char *m, struct feed *feed)
{
	struct item *item;

	if (strcmp(m, "FEED_TITLE") == 0)
		printf("%s", feed->title != NULL && *feed->title != '\0' ?
		    feed->title : "(none)");
	else if (strcmp(m, "ITEMS") == 0) {
		SLIST_FOREACH(item, &feed->items, next)
			parse_template(template_item, TYPE_ITEM, item);
	}
}

static void
item_markers(const char *m, struct item *item)
{
	struct category *cat;
	char buf[BUFSIZ];

	if (strcmp(m, "ITEM_TITLE") == 0) {
		printf("%s", item->title != NULL && *item->title != '\0' ?
		    item->title : "(none)");
	} else if (strcmp(m, "ITEM_LINK") == 0) {
		printf("%s", item->link);
	} else if (strcmp(m, "ITEM_DESCR") == 0) {
		printf("%s", item->descr);
	} else if (strcmp(m, "ITEM_AUTHOR") == 0) {
		printf("%s", item->author);
	} else if (strcmp(m, "ITEM_ID") == 0) {
		printf("%s", item->id);
	} else if (strcmp(m, "ITEM_DATE") == 0) {
		if (item->time != (time_t)-1) {
			strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT",
			    localtime(&item->time));
			printf("%s", buf);
		} else
			printf("%s (malformed)", item->date);
	} else if (strcmp(m, "ITEM_CATEGORIES") == 0) {
		SLIST_FOREACH(cat, &item->categories, next) {
			printf("%s", cat->name);
			if (SLIST_NEXT(cat, next) != NULL)
				printf(" / ");
		}
	}
}

static void
parse_line(char *line, int type, void *data)
{
	char *a, *b;

	for (a = line; (b = strstr(a, MARKER_TAG)) != NULL; a = b+2) {
		*b = '\0';
		printf("%s", a);
		a = b+2; 
		if ((b = strstr(a, MARKER_TAG)) != NULL) {
			*b = '\0';
			switch (type) {
			case TYPE_PAGE:
				page_markers(a, (struct feed *)data);
				break;
			case TYPE_FEED:
				feed_markers(a, (struct feed *)data);
				break;
			case TYPE_ITEM:
				item_markers(a, (struct item *)data);
			}
		}
	}
	printf("%s\n", a);
}

static void
parse_template(const char *file, int type, void *data)
{
	char buf[BUFSIZ], *a, *b;
	FILE *fin;

        if (file != NULL) {
                if ((fin = fopen(file, "r")) == NULL)
                        err(1, "fopen: %s", file);
                while (fgets(buf, sizeof(buf), fin) != NULL) {
                        buf[strcspn(buf, "\n")] = '\0';
                        parse_line(buf, type, data);
                }
	} else {
		switch (type) {
		case TYPE_PAGE:
			a = PAGE_TEMPLATE;
			break;
		case TYPE_FEED:
			a = FEED_TEMPLATE;
			break;
		case TYPE_ITEM:
			a = ITEM_TEMPLATE;
		}
		while (a != NULL) {
			if ((b = strchr(a, '\n')) != NULL) {
				memcpy(buf, a, b-a);
				buf[b-a] = '\0';
				parse_line(buf, type, data);
				++b;
			} else {
				memcpy(buf, a, strlen(a));
				parse_line(buf, type, data);
			}
			a = b;
		}
		
	}
}

void
output_html(struct feed *feeds, const char *args)
{
	template_page = template_feed = template_item = NULL;
	if (args != NULL && *args != '\0') {
		/* TODO custom templates */
	}
	parse_template(template_page, TYPE_PAGE, feeds);
}
