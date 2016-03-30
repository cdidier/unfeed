/*
 * Copyright (c) 2008 Colin Didier <cdidier@cybione.org>
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

void
print_text_item(struct feed *feed, struct item *item, FILE *stream)
{
	struct category *cat;
	char buf[BUFSIZ];

	fprintf(stream, "[%s] %s\n",
	    feed->title != NULL && *feed->title != '\0' ?
	    feed->title : "(none)",
	    item->title != NULL && *item->title != '\0' ?
	    item->title : "(none)");
	if (item->link != NULL && *item->link != '\0')
		fprintf(stream, "Link: %s\n", item->link);
	if (item->author != NULL && *item->author != '\0')
		fprintf(stream, "Author: %s\n", item->author);
	if (!SLIST_EMPTY(&item->categories)) {
		fprintf(stream, "Category: ");
		SLIST_FOREACH(cat, &item->categories, next) {
			fprintf(stream, "%s", cat->name);
			if (SLIST_NEXT(cat, next) != NULL)
				fprintf(stream, " / ");
			else
				fprintf(stream, "\n");
		}
	}
	if (item->time != (time_t)-1) {
		strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT",
		   localtime(&item->time)); 
		fprintf(stream, "Date: %s\n", buf);
	} else if (item->date != NULL && *item->date != '\0')
		fprintf(stream, "Date: %s (malformed)\n", item->date);
	if (item->descr != NULL && *item->descr != '\0')
		fprintf(stream, "\n%s\n", item->descr);
	fprintf(stream, "\n\n");
}

void
output_text(struct feed *feeds)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;
	struct item *item;

	SLIST_FIRST(&list) = feeds;
	SLIST_FOREACH(feed, &list, next) {
		SLIST_FOREACH(item, &feed->items, next) {
			if (!(item->flags & ITEM_OLD))
				print_text_item(feed, item, stdout);
		}
	}
}
