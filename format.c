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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "document.h"

void	strchomp(char *);
void	stroneline(char *);
int	insert_text(char **, const char *, size_t);

static char *
format_text(const char *cmd, const char *text)
{
	extern char **environ;
	char buf[BUFSIZ], *str;
	int pip_read[2], pip_write[2];
	size_t len;
	ssize_t nr;
	char *argp[] = {"sh", "-c", NULL, NULL};

	if (pipe(pip_read) == -1 || pipe(pip_write) == -1)
		err(1, "pipe");
	switch(fork()) {
	case 0:
		if (dup2(pip_read[1], STDOUT_FILENO) == -1)
			err(1, "dup2");
		if (dup2(pip_write[0], STDIN_FILENO) == -1)
			err(1, "dup2");
		close(pip_read[0]);
		close(pip_read[1]);
		close(pip_write[0]);
		close(pip_write[1]);
		argp[2] = (char *)cmd;
		if (execve("/bin/sh", argp, environ) == -1)
			err(1, "execve");
		exit(1);
	case -1:
		err(1, "fork");
	default:
		close(pip_read[1]);
		close(pip_write[0]);
		len = strlen(text);
		if (write(pip_write[1], text, len) < 0)
			err(1, "write");
		close(pip_write[1]);
		str = NULL;
		while ((nr = read(pip_read[0], buf, BUFSIZ)) != -1 && nr != 0) {
			if (insert_text(&str, buf, nr) == -1)
				err(1, "insert_text");
		}
		close(pip_read[0]);
		if (nr == -1)
			err(1, "read");
		wait(NULL);
	}
	return str;
}

void
format_feeds(struct feed *feeds)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;
	struct item *item;
	char *cmd, *str;

	if ((cmd = getenv("UNFEED_FORMAT")) == NULL || *cmd  == '\0')
		return;
	SLIST_FIRST(&list) = feeds;
	SLIST_FOREACH(feed, &list, next) {
		if (feed->title != NULL && *feed->title != '\0'
		    && (str = format_text(cmd, feed->title)) != NULL) {
			strchomp(str);
			stroneline(str);
			free(feed->title);
			feed->title = str;
		}
		SLIST_FOREACH(item, &feed->items, next) {
			if (item->title != NULL && *item->title != '\0'
			    && (str = format_text(cmd, item->title)) != NULL) {
				strchomp(str);
				stroneline(str);
				free(item->title);
				item->title = str;
			}
			if (item->descr != NULL && *item->descr != '\0'
			    && (str = format_text(cmd, item->descr)) != NULL) {
				free(item->descr);
				item->descr = str;
			}
			if (item->author != NULL && *item->author != '\0'
			    && (str = format_text(cmd, item->author)) != NULL) {
				strchomp(str);
				stroneline(str);
				free(item->author);
				item->author = str;
			}
		}
	}
}
