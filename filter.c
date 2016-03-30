/*
 * Copyright (c) 2009 Colin Didier <cdidier@cybione.org>
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
#include <time.h>

#include "document.h"

static int
is_new_item(struct item *item, struct tm *tm)
{
	struct tm *itm;

	if (item->time == (time_t)-1)
		return 0;
	itm = localtime(&item->time);
	if (itm->tm_year > tm->tm_year)
		return 1;
	if (itm->tm_year == tm->tm_year) {
		if (itm->tm_mon > tm->tm_mon)
			return 1;
		if (itm->tm_mon == tm->tm_mon) {
			if (itm->tm_mday > tm->tm_mday)
				return 1;
			if (itm->tm_mday == tm->tm_mday) {
				if (itm->tm_hour > tm->tm_hour)
					return 1;
				if (itm->tm_hour == tm->tm_hour) {
					if (itm->tm_min > tm->tm_min
					   || (itm->tm_min == tm->tm_min
					    && itm->tm_sec > tm->tm_sec))
						return 1;
				}
			}
		}
	}
	return 0;
}

void
filter_feeds(struct feed *feeds, struct tm *tm)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;
	struct item *item;

	SLIST_FIRST(&list) = feeds;
	SLIST_FOREACH(feed, &list, next) {
		SLIST_FOREACH(item, &feed->items, next) {
			if (!is_new_item(item, tm))
				item->flags |= ITEM_OLD;
		}
	}
}
