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

#define _XOPEN_SOURCE 500
#include <string.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TIME_FORMAT "%Y%m%d%H%M"

void run_url(char *);

struct config_feed {
	char		*url;
	struct tm	 lasttime;
	struct tm	 newtime;
	int		 ret;
	SLIST_ENTRY(config_feed) next;
};

static void
fork_feed(struct config_feed *feed)
{
	extern struct tm param_date;
	struct tm time_null;
	time_t now;

	time(&now);
	gmtime_r(&now, &feed->newtime);
	switch(fork()) {
	case 0:
		memset(&time_null, 0, sizeof(struct tm));
		if (memcmp(&param_date, &time_null, sizeof(struct tm)) == 0)
			memcpy(&param_date, &feed->lasttime,
			    sizeof(struct tm));
		run_url(feed->url);
		exit(0);
	case -1:
		err(1, "fork");
	default:
		wait(&feed->ret);
	}
}

static void
write_config(const char *config_file, struct config_feed *feeds)
{
	FILE *fin;
	SLIST_HEAD(, config_feed) list = SLIST_HEAD_INITIALIZER(list);
	struct config_feed *feed;
	struct tm time_null;
	char time[13];

	if ((fin = fopen(config_file, "w")) == NULL) {
		warn("%s", config_file);
		return;
	}
	SLIST_FIRST(&list) = feeds;
	fseek(fin, 0L, SEEK_SET);
	SLIST_FOREACH(feed, &list, next) {
		if (feed->ret != 0) {
			memset(&time_null, 0, sizeof(struct tm));
			if (memcmp(&feed->lasttime, &time_null,
			    sizeof(struct tm)) == 0)
				fprintf(fin, "%s\n", feed->url);
			else {
				strftime(time, sizeof(time), TIME_FORMAT,
				    &feed->lasttime);
				fprintf(fin, "%s\t%s\n", feed->url, time);
			}
		} else {
			strftime(time, sizeof(time), TIME_FORMAT,
			    &feed->newtime);
			fprintf(fin, "%s\t%s\n", feed->url, time);
		}
	}
	fclose(fin);
}

void
run_config(const char *config_file)
{
	FILE *fin;
	SLIST_HEAD(, config_feed) list = SLIST_HEAD_INITIALIZER(list);
	struct config_feed *feed, *tmp;
	char buf[BUFSIZ], *s, *t;
	int line;

	if ((fin = fopen(config_file, "r")) == NULL)
		err(1, "%s", config_file);
	line = 1;
	while (fgets(buf, sizeof(buf), fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		if ((s = strpbrk(buf, " \t")) != NULL) {
			if (*s != '\0') {
				*s++ = '\0';
				/* TODO shomptime */
			}
		}
		if ((feed = malloc(sizeof(struct config_feed))) == NULL)
			err(1, "malloc");
		if ((feed->url = strdup(buf)) == NULL)
			err(1, "strdup");
		if (s != NULL && *s != '\0') {
			t = strptime(s, "%Y%m%d%H%M", &feed->lasttime);
			if (t == NULL || (*t != '\0' && *t != '\n'))
				errx(1, "Malformed config file at line %d",
				    line);
		} else
			memset(&feed->lasttime, 0, sizeof(struct tm));
		feed->ret = -1;
		/* insert at the end of the list */
		tmp = NULL;
		SLIST_FOREACH(tmp, &list, next)
			if (SLIST_NEXT(tmp, next) == NULL)
				break;
		if (tmp != NULL)
			SLIST_INSERT_AFTER(tmp, feed, next);
		else
			SLIST_INSERT_HEAD(&list, feed, next);
		++line;
	}
	fclose(fin);
	SLIST_FOREACH(feed, &list, next) {
		fork_feed(feed);
		write_config(config_file, SLIST_FIRST(&list));
	}
	for (feed = SLIST_FIRST(&list); feed != NULL; feed = tmp) {
		tmp = SLIST_NEXT(feed, next);
		free(feed->url);
		free(feed);
	}
}
