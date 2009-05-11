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

#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define HTTP_URL	"http://"
#define HTTP_PORT	"80"
#define HTTP_USER_AGENT	"User-Agent: unfeed"
#define HTTP_LOCATION	"Location: "

#define EMPTYSTRING(s)	((s) == NULL || (*(s) == '\0'))

FILE *
request_url(char *url)
{
	char *newline, *host, *path, *credentials, *port, *cause;
	char buf[BUFSIZ];
	int s, error, save_errno;
	struct addrinfo hints, *res0, *res;
	FILE *fin;

	/* Extract URL parts */
	if ((newline = strdup(url)) == NULL)
		err(1, "strdup");
	if (strncasecmp(newline, HTTP_URL, sizeof(HTTP_URL)-1) != 0)	
		errx(1, "Not an URL: %s", url);
	host = newline + sizeof(HTTP_URL)-1;
	if (EMPTYSTRING(host))
		errx(1, "No host specified in the URL: %s", url);
	path = strchr(host, '/');
	if (!EMPTYSTRING(path))
		*path++ = '\0';
	if (EMPTYSTRING(path))
		path = NULL;
	credentials = strchr(host, '@');
	if (!EMPTYSTRING(credentials))
		host = credentials+1; /* ignore http credentials */
	port = strchr(host, ':');
	if (!EMPTYSTRING(port)) 
		*port++ = '\0';
	if (EMPTYSTRING(port))
		port = HTTP_PORT;

	/* Connect to the server */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(host, port, &hints, &res0);
	if (error == EAI_SERVICE)
		error = getaddrinfo(host, HTTP_PORT, &hints, &res0);
	if (error)
		errx(1, "getaddrinfo: %s", gai_strerror(error));
	s = -1;
	for (res = res0; res; res = res->ai_next) {
		if ((s = socket(res->ai_family, res->ai_socktype,
		    res->ai_protocol)) < 0) {
			cause = "socket";
			continue;
		}
again:
		if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
			if (errno == EINTR)
				goto again;
			save_errno = errno;
			close(s);
			errno = save_errno;
			s = -1;
			cause = "connect";
			continue;
		}
		break;
	}
	freeaddrinfo(res0);
	if (s < 0)
		err(1, cause);
	if ((fin = fdopen(s, "r+")) == NULL)
		err(1, "fdopen");

	/* Send request */
	if (fprintf(fin, "GET /%s HTTP/1.0\r\nHost: %s\r\n%s\r\n\r\n",
	    path != NULL ? path : "", host, HTTP_USER_AGENT) < 0)
		err(1, "fprintf");
	if (fflush(fin) == EOF)
		err(1, "Sending HTTP headers");
	free(newline);

	/* Parse headers */
	if (fgets(buf, sizeof(buf), fin) == NULL)
		err(1, "fgets");
	buf[strcspn(buf, "\n")] = '\0';
	/* TODO parse first line and extract codes */
	while (*buf != '\r' && *buf != '\0'
	    && fgets(buf, sizeof(buf), fin) != NULL) {
		buf[strcspn(buf, "\n")] = '\0';
		if (strncmp(buf, HTTP_LOCATION, sizeof(HTTP_LOCATION)-1) == 0) {
			fclose(fin);
			return request_url(buf+sizeof(HTTP_LOCATION)-1);
		}
		/* TODO parse other lines */
	}

	return fin;
}
