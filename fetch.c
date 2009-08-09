/*
 * $Id$
 *
 * Copyright (c) 2009 Colin Didier <cdidier@cybione.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __OpenBSD__
#define UNFEED_FETCH "ftp -o -"
#else
#define UNFEED_FETCH "curl"
#endif

FILE *
request_url(const char *url)
{
	extern char **environ;
	int pip[2];
	char *argp[] = {"sh", "-c", NULL, NULL };
	char *cmd, *fullcmd;
	size_t len;
	FILE *stream;

	if (pipe(pip) == -1)
		err(1, "pipe");
	switch(fork()) {
	case 0:
		if (dup2(pip[1], STDOUT_FILENO) == -1)
			err(1, "dup2");
		close(STDERR_FILENO);
		close(pip[0]);
		close(pip[1]);
		if ((cmd = getenv("UNFEED_FETCH")) == NULL)
			cmd = UNFEED_FETCH;
		len = strlen(cmd) + strlen(url) + 2;
		if ((fullcmd = malloc(len)) == NULL)
			err(1, "malloc");
		snprintf(fullcmd, len, "%s %s", cmd, url);
		argp[2] = fullcmd;
		if (execve("/bin/sh", argp, environ) == -1)
			err(1, "execve");
		free(fullcmd);
		exit(1);
	case -1:
		err(1, "fork");
	default:
		close(pip[1]);
		if ((stream = fdopen(pip[0], "r")) == NULL)
			err(1, "fdopen");
	}
	return stream;
}
