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

#define FORMAT_HTML \
	"<html><head>" \
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" \
	"</head><body>%s</body></html>"

static char *
format_text(const char *cmd, const char *text)
{
	extern char **environ;
	extern int param_fhtml;
	FILE *fin;
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
		if (param_fhtml) {
			if ((fin = fdopen(pip_write[1], "w")) == NULL)
				err(1, "fdopen");
			fprintf(fin, FORMAT_HTML, text);
			fclose(fin);
		} else {
			len = strlen(text);
			if (write(pip_write[1], text, len) < 0)
				err(1, "write");
		}
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
format_documents(struct document *docs)
{
	SLIST_HEAD(, document) list;
	struct document *d;
	struct article *a;
	char *cmd, *str;

	if ((cmd = getenv("UNFEED_FORMAT")) == NULL)
		return;
	SLIST_FIRST(&list) = docs;
	SLIST_FOREACH(d, &list, next) {
		if (*cmd != '\0' &&  d->title != NULL
		    && (str = format_text(cmd, d->title)) != NULL) {
			strchomp(str);
			stroneline(str);
			free(d->title);
			d->title = str;
		}
		SLIST_FOREACH(a, &d->articles, next) {
			if (*cmd != '\0' &&  a->title != NULL
			    && (str = format_text(cmd, a->title)) != NULL) {
				strchomp(str);
				stroneline(str);
				free(a->title);
				a->title = str;
			}
			if (*cmd != '\0' && a->descr != NULL
			    && (str = format_text(cmd, a->descr)) != NULL) {
				free(a->descr);
				a->descr = str;
			}
			if (*cmd != '\0' &&  a->author != NULL
			    && (str = format_text(cmd, a->author)) != NULL) {
				strchomp(str);
				stroneline(str);
				free(a->author);
				a->author = str;
			}
		}
	}
}
