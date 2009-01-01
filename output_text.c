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

#include "document.h"

void format_documents(struct document *);

static void
render_text_plain(struct document *docs)
{
	SLIST_HEAD(, document) list;
	struct document *d;
	struct article *a;

	SLIST_FIRST(&list) = docs;
	SLIST_FOREACH(d, &list, next) {
		SLIST_FOREACH(a, &d->articles, next) {
			printf("[%s] %s\n",
			    d->title != NULL && *d->title != '\0' ?
			    d->title : "(none)",
			    a->title != NULL && *a->title != '\0' ?
			    a->title : "(none)");
			if (a->link != NULL && *a->link != '\0')
				printf("Link: %s\n", a->link);
			if (a->author != NULL && *a->author != '\0')
				printf("Author: %s\n", a->author);
			if (a->date != NULL && *a->date != '\0')
				printf("Date: %s\n", a->date);
			if (a->descr != NULL && *a->descr != '\0')
				printf("\n%s\n", a->descr);
			printf("\n\n");
		}
	}
}

void
render_ouput(struct document *docs)
{
	extern char *param_output;
	
	format_documents(docs);
	if (param_output == NULL || strcasecmp(param_output, "plain") == 0)
		render_text_plain(docs);
}