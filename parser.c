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
time_t	rfc822_date(const char *);

#define RSS_DOC		"channel"
#define RSS_ARTICLE	"item"
#define RSS_TITLE	"title"
#define RSS_LINK	"link"
#define RSS_DESCR	"description"
#define RSS_AUTHOR	"author"
#define RSS_ID		"guid"
#define RSS_DATE	"pubDate"
#define RSS_CATEGORY	"category"
#define RSS_ENCLOSURE	"enclosure"

static SLIST_HEAD(, document) documents;
static struct document	*cd; /* current document */
static struct article	*ca; /* current article */
static int	 depth;
static int	 read_data;
static char	*data;

static void XMLCALL
rss_start_elt(void *user_data, const char *name, const char **atts)
{
	switch (depth++) {
	case 0:
		if (strcasecmp(name, RSS_DOC) == 0) {
			if ((cd = malloc(sizeof(struct document))) == NULL)
				err(1, "malloc");
			INIT_DOCUMENT(cd);
			SLIST_INSERT_HEAD(&documents, cd, next);
		}
		break;
	case 1:
		if (cd == NULL)
			break;
		if (strcasecmp(name, RSS_TITLE) == 0)
			read_data = 1;
		else if (strcasecmp(name, RSS_ARTICLE) == 0) {
			if ((ca = malloc(sizeof(struct article))) == NULL)
				err(1, "malloc");
			INIT_ARTICLE(ca);
			SLIST_INSERT_HEAD(&cd->articles, ca, next);
		}
		break;
	case 2:
		if (ca == NULL)
			break;
		if ((ca->title == NULL && strcasecmp(name, RSS_TITLE) == 0)
		    || (ca->link == NULL && strcasecmp(name, RSS_LINK) == 0)
		    || (ca->descr == NULL && strcasecmp(name, RSS_DESCR) == 0)
		    || (ca->author == NULL && strcasecmp(name, RSS_AUTHOR) == 0)
		    || (ca->id == NULL && strcasecmp(name, RSS_ID) == 0)
		    || (ca->date == NULL && strcasecmp(name, RSS_DATE) == 0)
		    || (strcasecmp(name, RSS_CATEGORY) == 0))
			read_data = 1;
		else if (strcasecmp(name, RSS_ENCLOSURE) == 0) {
			/* TODO parse atts */
		}
		break;
	}
}

static void XMLCALL
rss_data_handler(void *user_data, const XML_Char *s, int len)
{
	if (read_data != 0)
		if (insert_text(&data, s, (size_t)len) == -1)
			err(1, "insert_text");
}

static void XMLCALL
rss_end_elt(void *user_data, const char *name)
{
	switch (--depth) {
	case 0:
		if (strcasecmp(name, RSS_DOC) == 0)
			cd = NULL;
		break;
	case 1:
		if (strcasecmp(name, RSS_ARTICLE) == 0) {
			ca = NULL;
			break;
		}
		if (data == NULL || cd == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, RSS_TITLE) == 0)
			cd->title = data;
		else
			free(data);
		goto out;
	case 2:
		if (data == NULL || ca == NULL)
			break;
		strchomp(data);
		if (strcasecmp(name, RSS_TITLE) == 0)
			ca->title = data;
		else if (strcasecmp(name, RSS_LINK) == 0)
			ca->link = data;
		else if (strcasecmp(name, RSS_DESCR) == 0)
			ca->descr = data;
		else if (strcasecmp(name, RSS_AUTHOR) == 0)
			ca->author = data;
		else if (strcasecmp(name, RSS_ID) == 0)
			ca->id = data;
		else if (strcasecmp(name, RSS_DATE) == 0) {
			ca->date = data;
			ca->time = rfc822_date(data);
		} else if (strcasecmp(name, RSS_CATEGORY) == 0) {
			struct category *cat;

			if ((cat = malloc(sizeof(struct category))) == NULL)
				err(1, "malloc");
			cat->name = data;
			SLIST_INSERT_HEAD(&ca->categories, cat, next);
		} else
			free(data);
	}
out:	read_data = 0;
	data = NULL;
}

static void XMLCALL
start_elt(void *user_data, const char *name, const char **atts)
{
	XML_Parser parser = (XML_Parser)user_data;
	int i, supported;

	supported = 0;
        if (strcasecmp(name, "rss") == 0) {
                supported = 1;
		XML_SetElementHandler(parser, rss_start_elt, rss_end_elt);
                XML_SetCharacterDataHandler(parser, rss_data_handler);
        } else if (strcasecmp(name, "feed") == 0) {
                for (i = 0; atts[i] && !supported; i += 2)
                        if (strcasecmp(atts[i], "xmlns") == 0
                            && strcmp(atts[i+1],
                            "http://www.w3.org/2005/Atom") == 0)
                                supported = 1;
                if (supported) {
                        errx(1, "Atom feeds not yet supported");
                }
        }
	if (supported) {
		SLIST_INIT(&documents);
		cd = NULL;
		ca = NULL;
		depth = 0;
		read_data = 0;
		data = NULL;
        } else
                errx(1, "Unsupported feed");
}

struct document *
parse_document(FILE *fin)
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
	return SLIST_FIRST(&documents);
}
