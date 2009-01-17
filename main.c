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

void		 run_config(const char *);
FILE		*request_url(char *);
struct document	*parse_document(FILE *);
void		 format_documents(struct document *);
void		 output_html(struct document *, const char *);
void		 output_text(struct document *, const char *);

void		(*output)(struct document *, const char *);
char		*output_args;
struct tm	 param_time;

static void
usage(void)
{
	fprintf(stderr, "usage: unfeed [-t time] [-o output_module] -f file\n"
			"       unfeed [-t time] [-o output_module] url\n");
	exit(1);
}

void
run_url(char *url)
{
	FILE *fin;
	struct document *docs;
	
	fin = request_url(url);
	docs = parse_document(fin);
	format_documents(docs);
	fclose(fin);
	if (output != NULL)
		output(docs, output_args);
}

int
main(int argc, char *argv[])
{
	char ch, *config_file, *t;

	output = output_text;
	output_args = config_file = NULL;
	memset(&param_time, 0, sizeof(struct tm));
	while ((ch = getopt(argc, argv, "f:o:t:")) != -1) {
		switch (ch) {
		case 'f':
			config_file = optarg;
			break;
		case 'o':
			if (strncmp(optarg, "html", 4) == 0) {
				output = output_html;
				output_args = optarg+4;
			} else if (strncmp(optarg, "text", 4) == 0)
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
