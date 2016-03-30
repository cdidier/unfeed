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

#define _XOPEN_SOURCE
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "document.h"

void		 run_config(const char *);
FILE		*request_url(char *);
struct feed	*parse_feeds(FILE *);
void		 filter_feeds(struct feed *, struct tm *tm);
void		 format_feeds(struct feed *);
void		 output_cmd(struct feed *);
void		 output_html(struct feed *);
void		 output_mail(struct feed *);
void		 output_text(struct feed *);

void		(*output)(struct feed *);
struct tm	 param_date;

static void
usage(void)
{
	fprintf(stderr, "usage: unfeed [-d date] [-o output] -f file\n"
			"       unfeed [-d date] [-o output] url\n");
	exit(1);
}

void
run_url(char *url)
{
	FILE *fin;
	struct feed *feeds;
	
	if (strcmp(url, "-") == 0) {
		fprintf(stderr, "Checking from stdin...\n");
		fin = stdin;
	} else {
		fprintf(stderr, "Checking %s...\n", url); 
		fin = request_url(url);
	}
	feeds = parse_feeds(fin);
	filter_feeds(feeds, &param_date);
	format_feeds(feeds);
	fclose(fin);
	if (output != NULL)
		output(feeds);
}

int
main(int argc, char *argv[])
{
	char ch, *config_file, *d;

	output = output_text;
	config_file = NULL;
	memset(&param_date, 0, sizeof(struct tm));
	while ((ch = getopt(argc, argv, "f:o:d:")) != -1) {
		switch (ch) {
		case 'f':
			config_file = optarg;
			break;
		case 'o':
			if (strncmp(optarg, "cmd", 3) == 0)
				output = output_cmd;
			else if (strncmp(optarg, "html", 4) == 0)
				output = output_html;
			else if (strncmp(optarg, "mail", 4) == 0)
				output = output_mail;
			else if (strncmp(optarg, "text", 4) == 0)
				output = output_text;
			else if (strncmp(optarg, "null", 4) == 0)
				output = NULL;
			else
				errx(1, "Unknown output module");
			break;
		case 'd':
			d = strptime(optarg, "%Y%m%d%H%M", &param_date);
			if (d == NULL || (*d != '\0' && *d != '\n'))
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
