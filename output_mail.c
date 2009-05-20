/*
 * $Id$
 *
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
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "document.h"

#define UNFEED_MAILCMD	"sendmail -eom %s"

static void
format_mail(const char *mail, struct feed *feed, struct item *item, int fd)
{
	FILE *fout;
	struct category *cat;
	char buf[BUFSIZ];

	if ((fout = fdopen(fd, "w")) == NULL)
		err(1, "fdopen");
	if (gethostname(buf, sizeof(buf)) == -1)
		err(1, "gethostname");
	fprintf(fout, "From: %s <unfeed@%s>\n",
		feed->title != NULL && *feed->title != '\0' ?
		feed->title : "(none)", buf);
	fprintf(fout, "To: %s\n", mail);
	fprintf(fout, "Subject: [%s] %s\n",
	    feed->title != NULL && *feed->title != '\0' ?
	    feed->title : "(none)",
	    item->title != NULL && *item->title != '\0' ?
	    item->title : "(none)");
	if (item->time != (time_t)-1) {
		strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT",
		    localtime(&item->time));
		fprintf(fout, "Date: %s\n", buf);
	} else if (item->date != NULL && *item->date != '\0')
		printf("Date: %s\n", item->date);
	if (item->link != NULL && *item->link != '\0')
		fprintf(fout, "Link: %s\n", item->link);
	if (item->author != NULL && *item->author != '\0')
		fprintf(fout, "Author: %s\n", item->author);
	if (!SLIST_EMPTY(&item->categories)) {
		fprintf(fout, "Category: ");
		SLIST_FOREACH(cat, &item->categories, next) {
			fprintf(fout, "%s", cat->name);
			if (SLIST_NEXT(cat, next) != NULL)
				fprintf(fout, " / ");
			else
				fprintf(fout, "\n");
		}
	}
	if (item->time != (time_t)-1) {
		strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT",
		    localtime(&item->time));
		fprintf(fout, "Date: %s\n", buf);
	} else if (item->date != NULL && *item->date != '\0')
		fprintf(fout, "Date: %s (malformed)\n", item->date);
	if (item->descr != NULL && *item->descr != '\0')
		fprintf(fout, "\n%s\n", item->descr);
	fclose(fout);
}

static void
send_mail(const char *cmd, const char *mail, struct feed *feed,
    struct item *item)
{
	extern char **environ;
	int pip[2];
	char *argp[] = {"sh", "-c", (char *)cmd, NULL};

	if (pipe(pip) == -1)
		err(1, "pipe");
	switch(fork()) {
	case 0:
		if (dup2(pip[0], STDIN_FILENO) == -1)
			err(1, "dup2");
		close(pip[1]);
		if (execve("/bin/sh", argp, environ) == -1)
			err(1, "execve");
		close(pip[0]);
		exit(1);
	case -1:
		err(1, "fork");
	default:
		close(pip[0]);
		format_mail(mail, feed, item, pip[1]);
		close(pip[1]);
		wait(NULL);
	}
}

void
output_mail(struct feed *feeds, const char *args)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;
	struct item *item;
	char *mail, *cmd, buf[BUFSIZ];

	mail = getenv("USER");
	if ((cmd = getenv("UNFEED_MAILCMD")) == NULL) {
		snprintf(buf, sizeof(buf), UNFEED_MAILCMD, mail);
		cmd = buf;
	}
	SLIST_FIRST(&list) = feeds;
	SLIST_FOREACH(feed, &list, next)
		SLIST_FOREACH(item, &feed->items, next) {
			if (!(item->flags & ITEM_OLD))
				send_mail(cmd, mail, feed, item);
		}
}
