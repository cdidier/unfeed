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

/* from output_text.c */
void print_text_item(struct feed *, struct item *, FILE *);

static void
run_cmd(const char *cmd, struct feed *feed, struct item *item)
{
	extern char **environ;
	int pip[2];
	char *argp[] = {"sh", "-c", (char *)cmd, NULL};
	FILE *stream;

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
		if ((stream = fdopen(pip[1], "w")) == NULL)
			err(1, "fdopen");
		print_text_item(feed, item, stream);
		fclose(stream);
		close(pip[1]);
		wait(NULL);
	}
}

void
output_cmd(struct feed *feeds)
{
	SLIST_HEAD(, feed) list;
	struct feed *feed;
	struct item *item;
	char *cmd;

	if ((cmd = getenv("UNFEED_CMD")) == NULL)
		err(1, "no UNFEED_CMD command specified");
	SLIST_FIRST(&list) = feeds;
	SLIST_FOREACH(feed, &list, next)
		SLIST_FOREACH(item, &feed->items, next) {
			if (!(item->flags & ITEM_OLD))
				run_cmd(cmd, feed, item);
		}
}
