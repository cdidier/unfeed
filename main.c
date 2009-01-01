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

#include "document.h"

FILE		*request_url(char *);
struct document	*parse_document(FILE *);
void		 run_config(const char *);
void		 output_text(struct document *, const char *);

void		(*output)(struct document *, const char *);
char		*param_output;
struct tm	 param_time;
int		 param_fhtml;

static void
usage(void)
{
	fprintf(stderr, "usage: unfeed [-t time] -f file\n"
			"       unfeed [-t time] url\n");
	exit(1);
}

void
run_url(char *url)
{
	FILE *fin;
	struct document *docs;
	
	fin = request_url(url);
	docs = parse_document(fin);
	fclose(fin);
	if (output != NULL)
		output(docs, param_output);
}

int
main(int argc, char *argv[])
{
	char ch, *config_file, *t;

	output = output_text;
	memset(&param_time, 0, sizeof(struct tm));
	param_output = config_file = NULL;
	param_fhtml = 0;
	while ((ch = getopt(argc, argv, "f:ho:t:")) != -1) {
		switch (ch) {
		case 'f':
			config_file = optarg;
			break;
		case 'h':
			param_fhtml = 1;
			break;
		case 'o':
			if (strncmp(optarg, "text", 4) == 0)
				output = output_text;
			else if (strncmp(optarg, "null", 4) == 0)
				output = NULL;
			else
				errx(1, "Unknown output");
			break;
		case 't':
			t = strptime(optarg, "%Y%m%d%H%M", &param_time);
			if (t == NULL || (*t != '\0' && *t != '\n'))
				errx(1, "Malformed time value");
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (config_file != NULL && argc == 0)
		run_config(config_file);
	else if (config_file == NULL && argc == 1)
		run_url(*argv);
	else 
		usage();
	return 0;
}
