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
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void
strchomp(char *s)
{
	size_t i;

	/* remove spaces at the end of the string */
	for (i = strlen(s); i > 0 && isspace(s[i-1]); --i);
	s[i] = '\0';
	/* remove spaces at the beginning of the string */
	for (i = 0; s[i] != '\0' && isspace(s[i]); ++i);
	if (i > 0)
		for (; *s != '\0'; ++s)
			*s = *(s+i);
}

void
stroneline(char *s)
{
	size_t len, i, spaces;
	char *p1, *p2;
	
	for (p1 = s; *p1 != '\0'; ++p1) {
		if (isspace(*p1)) {
			*p1 = ' ';
			p2 = p1;
			spaces = 0;
			while (*(p2++) != '\0' && isspace(*p2))
				++spaces;
			if (spaces > 0) {
				len = strlen(p1);
				for (i = 0; i < len-spaces+1; ++i)
					p1[i-spaces] = p1[i];
			}
		}
	}
}

int
insert_text(char **s, const char *text, size_t len)
{
	size_t oldlen;
	char *tmp;

	if (len == 0)
		return 0;
	if (*s == NULL) {
		if ((*s = calloc(len+1, sizeof(char))) == NULL)
			return -1;
		strncpy(*s, text, len);
		(*s)[len] = '\0';
	} else {
		oldlen = strlen(*s);
		if (oldlen && (len+1) > SIZE_MAX / oldlen) {
			errno = ENOMEM;
			return -1;
		}
		if ((tmp = realloc(*s, oldlen+len+1)) == NULL)
			return -1;
		*s = tmp;
		strncpy(*s+oldlen, text, len);
		(*s)[oldlen+len] = '\0';
	}
	return 0;
}
