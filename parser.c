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

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <expat.h>

#include "document.h"

void	strchomp(char *);
int	insert_text(char **, const char *, size_t);
time_t	rfc822_date(char *);
time_t	rfc3339_date(char *);

#define RSS		"rss"
#define RSS_CHANNEL	"channel"
#define RSS_ITEM	"item"
#define RSS_TITLE	"title"
#define RSS_LINK	"link"
#define RSS_DESCR	"description"
#define RSS_AUTHOR	"author"
#define RSS_ID		"guid"
#define RSS_DATE	"pubDate"
#define RSS_CATEGORY	"category"
#define RSS_ENCLOSURE	"enclosure"

#define ATOM_XMLNS	"http://www.w3.org/2005/Atom"
#define ATOM_FEED	"feed"
#define ATOM_ENTRY	"entry"
#define ATOM_TITLE	RSS_TITLE
#define ATOM_CONTENT	"content"
#define ATOM_LINK	RSS_LINK
#define ATOM_AUTHOR	RSS_AUTHOR
#define ATOM_NAME	"name"
#define ATOM_ID		"id"
#define ATOM_UPDATED	"updated"

#define RDF		"rdf:RDF"

static SLIST_HEAD(, feed) feeds;
static struct feed	*cf; /* current feed */
static struct item	*ci; /* current item */
static int	 depth;
static int	 read_data;
static char	*data;

static void XMLCALL
rss_start_elt(void *user_data, const char *name, const char **atts)
{
	switch (depth++) {
	case 0:
		if (strcasecmp(name, RSS_CHANNEL) == 0) {
			if ((cf = malloc(sizeof(struct feed))) == NULL)
				err(1, "malloc");
			INIT_FEED(cf);
			SLIST_INSERT_HEAD(&feeds, cf, next);
		}
		break;
	case 1:
		if (cf == NULL)
			break;
		if (strcasecmp(name, RSS_TITLE) == 0)
			read_data = 1;
		else if (strcasecmp(name, RSS_ITEM) == 0) {
			if ((ci = malloc(sizeof(struct item))) == NULL)
				err(1, "malloc");
			INIT_ITEM(ci);
			SLIST_INSERT_HEAD(&cf->items, ci, next);
		}
		break;
	case 2:
		if (ci == NULL)
			break;
		if ((ci->title == NULL && strcasecmp(name, RSS_TITLE) == 0)
		    || (ci->link == NULL && strcasecmp(name, RSS_LINK) == 0)
		    || (ci->descr == NULL && strcasecmp(name, RSS_DESCR) == 0)
		    || (ci->author == NULL && strcasecmp(name, RSS_AUTHOR) == 0)
		    || (ci->id == NULL && strcasecmp(name, RSS_ID) == 0)
		    || (ci->date == NULL && strcasecmp(name, RSS_DATE) == 0)
		    || (strcasecmp(name, RSS_CATEGORY) == 0))
			read_data = 1;
		else if (strcasecmp(name, RSS_ENCLOSURE) == 0) {
			struct enclosure *e;
			int i;

			if ((e = malloc(sizeof(struct enclosure))) == NULL)
				err(1, "malloc");
			INIT_ENCLOSURE(e);
			SLIST_INSERT_HEAD(&ci->enclosures, e, next);
			for (i = 0; atts[i] != NULL; i += 2) {
				if (strcmp(atts[i], "url") == 0) {
					if ((e->url = strdup(atts[i+1])) == NULL)
						err(1, "strdup");
				} else if (strcmp(atts[i], "length") == 0) {
					if ((e->size = strdup(atts[i+1])) == NULL)
						err(1, "strdup");
				} else if (strcmp(atts[i], "type") == 0) {
					if ((e->type = strdup(atts[i+1])) == NULL)
						err(1, "strdup");
				}
			}
		}
	}
}

static void XMLCALL
rss_end_elt(void *user_data, const char *name)
{
	switch (--depth) {
	case 0:
		if (strcasecmp(name, RSS_CHANNEL) == 0)
			cf = NULL;
		break;
	case 1:
		if (strcasecmp(name, RSS_ITEM) == 0) {
			ci = NULL;
			break;
		}
		if (data == NULL || cf == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, RSS_TITLE) == 0)
			cf->title = data;
		else
			free(data);
		goto out;
	case 2:
		if (data == NULL || ci == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, RSS_TITLE) == 0)
			ci->title = data;
		else if (strcasecmp(name, RSS_LINK) == 0)
			ci->link = data;
		else if (strcasecmp(name, RSS_DESCR) == 0)
			ci->descr = data;
		else if (strcasecmp(name, RSS_AUTHOR) == 0)
			ci->author = data;
		else if (strcasecmp(name, RSS_ID) == 0)
			ci->id = data;
		else if (strcasecmp(name, RSS_DATE) == 0) {
			ci->date = data;
			ci->time = rfc822_date(data);
		} else if (strcasecmp(name, RSS_CATEGORY) == 0) {
			struct category *cat;

			if ((cat = malloc(sizeof(struct category))) == NULL)
				err(1, "malloc");
			cat->name = data;
			SLIST_INSERT_HEAD(&ci->categories, cat, next);
		} else
			free(data);
	}
out:	read_data = 0;
	data = NULL;
}

static void XMLCALL
atom_start_elt(void *user_data, const char *name, const char **atts)
{
	switch (depth++) {
	case 0:
		if (strcasecmp(name, ATOM_TITLE) == 0)
			read_data = 1;
		else if (strcasecmp(name, ATOM_ENTRY) == 0) {
			if ((ci = malloc(sizeof(struct item))) == NULL)
				err(1, "malloc");
			INIT_ITEM(ci);
			SLIST_INSERT_HEAD(&cf->items, ci, next);
		}
		break;
	case 1:
		if (ci == NULL)
			break;
		if ((ci->title == NULL && strcasecmp(name, ATOM_TITLE) == 0)
		    || (ci->link == NULL && strcasecmp(name, ATOM_LINK) == 0)
		    || (ci->descr == NULL && strcasecmp(name, ATOM_CONTENT) == 0)
		    || (ci->id == NULL && strcasecmp(name, ATOM_ID) == 0)
		    || (ci->author == NULL && strcasecmp(name, ATOM_AUTHOR) == 0)
		    || (ci->date == NULL && strcasecmp(name, ATOM_UPDATED) == 0))
			read_data = 1;
		break;
	case 2:
		break;
	}
}

static void XMLCALL
atom_end_elt(void *user_data, const char *name)
{
	switch (--depth) {
	case 0:
		if (strcasecmp(name, ATOM_ENTRY) == 0) {
			ci = NULL;
			break;
		}
		if (data == NULL || cf == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, ATOM_TITLE) == 0)
			cf->title = data;
		else
			free(data);
		goto out;
	case 1:
		if (data == NULL || ci == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, ATOM_TITLE) == 0)
			ci->title = data;
		else if (strcasecmp(name, ATOM_LINK) == 0)
			ci->link = data;
		else if (strcasecmp(name, ATOM_CONTENT) == 0)
			ci->descr = data;
		else if (strcasecmp(name, ATOM_AUTHOR) == 0)
			ci->author = data;
		else if (strcasecmp(name, ATOM_ID) == 0)
			ci->id = data;
		else if (strcasecmp(name, ATOM_UPDATED) == 0) {
			ci->date = data;
			ci->time = rfc3339_date(data);
		} else
			free(data);
		goto out;
	case 2:
	}
out:	read_data = 0;
	data = NULL;
}

static void XMLCALL
rdf_start_elt(void *user_data, const char *name, const char **atts)
{
	switch (depth++) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	}
}

static void XMLCALL
rdf_end_elt(void *user_data, const char *name)
{
	switch (--depth) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		break;
	}
}

static void XMLCALL
data_handler(void *user_data, const XML_Char *s, int len)
{
	if (read_data != 0)
		if (insert_text(&data, s, (size_t)len) == -1)
			err(1, "insert_text");
}

static void XMLCALL
start_elt(void *user_data, const char *name, const char **atts)
{
	XML_Parser parser = (XML_Parser)user_data;
	int i, supported;

	supported = 0;
	cf = NULL;
	ci = NULL;
	depth = 0;
	read_data = 0;
	data = NULL;
	SLIST_INIT(&feeds);
	if (strcasecmp(name, RSS) == 0) {
		supported = 1;
		XML_SetElementHandler(parser, rss_start_elt, rss_end_elt);
	} else if (strcasecmp(name, ATOM_FEED) == 0) {
		for (i = 0, supported = 0 ; atts[i] && !supported; i += 2)
			if (strcasecmp(atts[i], "xmlns") == 0
			    && strcmp(atts[i+1], ATOM_XMLNS) == 0) {
				supported = 1;
				XML_SetElementHandler(parser, atom_start_elt,
				    atom_end_elt);
				if ((cf = malloc(sizeof(struct feed))) == NULL)
					err(1, "malloc");
				INIT_FEED(cf);
				SLIST_INSERT_HEAD(&feeds, cf, next);
			}
	} else if (strcasecmp(name, RDF) == 0) {
		supported = 1;
		XML_SetElementHandler(parser, rdf_start_elt, rdf_end_elt);
		errx(1, "RDF feeds not yed supported");
	}
	if (!supported)
		errx(1, "Unsupported feed");
	XML_SetCharacterDataHandler(parser, data_handler);
}

struct feed *
parse_feeds(FILE *fin)
{
	XML_Parser parser;
	char buf[BUFSIZ];
	size_t len;
	int done;

	parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, parser);
	XML_SetStartElementHandler(parser, start_elt);
	done = 0;
	do {
		len = fread(buf, sizeof(char), BUFSIZ, fin);
		if (ferror(fin))
			err(1, "Read error");
		done = feof(fin);
		if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR)
			errx(1, "Parse error at line %lu: %s",
			    XML_GetCurrentLineNumber(parser),
			    XML_ErrorString(XML_GetErrorCode(parser)));
	} while (!done);
	XML_ParserFree(parser);
	return SLIST_FIRST(&feeds);
}
