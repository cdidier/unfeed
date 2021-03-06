/*
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

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <stdio.h>

static void
strchomp_begin(char *s)
{
	size_t i;

	for (i = 0; s[i] != '\0' && isspace(s[i]); ++i);
	if (i > 0) {
		for (; s[i] != '\0'; ++s)
			*s = s[i];
		*s = '\0';
	}
}

static void
strchomp_end(char *s)
{
	size_t i;

	for (i = strlen(s); i > 0 && isspace(s[i-1]); --i);
	s[i] = '\0';
}

void
strchomp_lines(char *s)
{
	size_t i;
	char *p1, *p2;
	int have_char;

	have_char = 0;
	while ((p1 = strpbrk(s, "\n\r")) != NULL && !have_char) {
		for (p2 = s; p2 < p1 && !have_char; ++p2)
			if (!isspace(*p2))
				have_char = 1;
		if (!have_char) {
			for (i = 0; s[i+p2-s+1] != '\0'; ++i)
				s[i] = s[i+p2-s+1];
			s[i] = '\0';
		}
		p1 = p2+1;
	}
	strchomp_end(s);
}


void
strchomp(char *s)
{
	strchomp_begin(s);
	strchomp_end(s);
}

void
stroneline(char *s)
{
	for (; *s != '\0'; ++s)
		if (isspace(*s)) {
			*s = ' ';
			strchomp_begin(s+1);
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


static long
parse_timezone_rfc822(const char *tz)
{
	const char *rfc822_timezones[26][4] = {
		{ "M", NULL },			/* UTC-12 */
		{ "L", NULL },
		{ "K", NULL },
		{ "I", NULL },
		{ "H", "PST", NULL }, 		/* UTC-8 */
		{ "G", "MST", "PDT", NULL },	/* UTC-7 */
		{ "F", "CST", "MDT", NULL },	/* UTC-6 */
		{ "E", "EST", "CDT", NULL },	/* UTC-5 */
		{ "D", "EDT", NULL },		/* UTC-4 */
		{ "C", NULL },
		{ "B", NULL },
		{ "A", NULL },
		{ "Z", "UT", "GMT", NULL },	/* UTC */
		{ "N", NULL },
		{ "O", NULL },
		{ "P", NULL },
		{ "Q", NULL },
		{ "R", NULL },
		{ "S", NULL },
		{ "T", NULL },
		{ "U", NULL },
		{ "V", NULL },
		{ "W", NULL },
		{ "X", NULL },
		{ "Y", NULL },			/* UTC+12 */
		{ NULL }
	};
	long i, j;

	if ((*tz == '+' || *tz == '-') && strlen(tz) == 5) {
		i = atoi(tz);
		return ((i/100)*60 + i%100) * 60;
	}
	for (i = 0; rfc822_timezones[i] != NULL; ++i)
		for (j = 0; rfc822_timezones[i][j] != NULL; ++j)
			if (strcmp(rfc822_timezones[i][j], tz) == 0)
				return (i - 12) * 3600;
	return 0;
}

time_t
rfc822_date(char *date)
{
	struct tm tm;
	long offset;
	char *p;
	int i;
	char *formats[] = { "%d %b %Y %T", "%d %b %Y %H:%M", "%d %b %y %T",
	     "%d %b %y %H:%M", NULL };

	memset(&tm, 0, sizeof(struct tm));
	if ((p = strchr(date, ',')) != NULL)
		date = p+2; /* ignore day of the week */
	strchomp(date);
	for (i = 0; formats[i] != NULL
	    && (p = strptime(date, formats[i], &tm)) == NULL; ++i);
	if (p == NULL)
		return (time_t)-1;
	strchomp(p);
	tm.tm_isdst = -1;
	tm.tm_gmtoff = offset = *p != '\0' ? parse_timezone_rfc822(p) : 0;
	return mktime(&tm) - offset;
}

static long
parse_timezone_rfc3339(const char *tz)
{
	struct tm tm;

	if ((*tz == '+' || *tz == '-') && strlen(tz) == 6) {
		if (strptime(tz+1, "%H:%M",  &tm) == 0)
			return 0;
		return (*tz == '+' ? 1 : -1) * (tm.tm_hour*60+tm.tm_min)*60;
	}
	return 0;
}

time_t
rfc3339_date(char *date)
{
	struct tm tm;
	long offset;
	char *p;

	memset(&tm, 0, sizeof(struct tm));
	strchomp(date);
	if ((p = strptime(date, "%Y-%m-%dT%H:%M:%S", &tm)) == NULL)
		return (time_t)-1;
	if (*p == '.' && strlen(p) >= 3) /* ignore fractional secondi */
		p += 3;
	tm.tm_isdst = -1;
	tm.tm_gmtoff = offset = *p != '\0' ? parse_timezone_rfc3339(p) : 0;
	return mktime(&tm) - offset;
}
