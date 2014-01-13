/* -*- c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
/*
 * Copyright (c) 2009-2014 Ralf Horstmann <ralf@ackstorm.de>
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

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nxt.h"

int Bflag, bflag, dflag, fflag, gflag, iflag, lflag, pflag, vflag,
	startflag, stopflag;
char *filename;

int main(int argc, char *argv[]){
	int ch;
	int commands = 0;
	int status = 0;

	while ((ch = getopt(argc, argv, "BbdfghilpsSv")) != -1) {
		switch (ch) {
		case 'B':
			Bflag = 1;
			commands++;
			break;
		case 'b':
			bflag = 1;
			commands++;
			break;
		case 'd':
			dflag = 1;
			commands++;
			break;
		case 'f':
			fflag = 1;
			commands++;
			break;
		case 'g':
			gflag = 1;
			commands++;
			break;
		case 'i':
			iflag = 1;
			commands++;
			break;
		case 'l':
			lflag = 1;
			commands++;
			break;
		case 'p':
			pflag = 1;
			commands++;
			break;
		case 's':
			startflag = 1;
			commands++;
			break;
		case 'S':
			stopflag = 1;
			commands++;
			break;
		case 'v':
			vflag++;
			break;
		case 'h':
		default:
			(void)fprintf(stderr,
                          "usage: nxtctl [-BbdfghilpsSv] [filename/pattern]\n"
                          "        -B             boot (disabled by default)\n"
                          "        -b             print battery level\n"
                          "        -d [filename]  delete file\n"
                          "        -f             print firmware version\n"
                          "        -g [filename]  get file\n"
                          "        -p [filename]  put file\n"
                          "        -i             print device info\n"
                          "        -l [pattern]   list files\n"
                          "        -s [filename]  start program\n"
                          "        -S             stop running program\n"
                          "        -v             verbose debug output\n");
			exit(1);
			/* NOTREACHED */
		}
	}
	argv += optind;
	argc -= optind;
	if (argc > 0 && argv[0]) {
		filename = argv[0];
	}

	if (commands == 0) {
		fprintf(stderr, "error: no command option given\n");
		exit(1);
	}

	if (commands > 1) {
		fprintf(stderr, "error: multiple command options given\n");
		exit(1);
	}

	if (filename && vflag) {
		fprintf(stderr, "filename: %s argc: %d\n", filename, argc);
	}
	
	NXT *nxt = nxt_new();
	if (nxt_init(nxt) != 0) {
		exit(1);
	}
  
	if (Bflag) {
#if DANGEROUS
		status += nxt_boot(nxt);
#endif
	}

	if (bflag) {
		status += nxt_print_battery_level(nxt);
	}

	if (dflag) {
		if (!filename) {
			fprintf(stderr, "error: filename is mandatory\n");
			status = -1;
		} else {
			status += nxt_delete_file(nxt, filename);
		}
	}

	if (fflag) {
		status += nxt_print_firmware_version(nxt);
	}

	if (gflag) {
		if (!filename) {
			fprintf(stderr, "error: filename is mandatory\n");
			status = -1;
		} else {
			status += nxt_get_file(nxt, filename);
		}
	}

	if (pflag) {
		if (!filename) {
			fprintf(stderr, "error: filename is mandatory\n");
			status = -1;
		} else {
			status += nxt_put_file(nxt, filename);
		}
	}

	if (iflag) {
		status += nxt_print_device_info(nxt);
	}

	if (lflag) {
		if (! filename) {
			filename = "*.rxe";
		}
		status += nxt_print_files(nxt, filename);
	}

	if (startflag) {
		if (!filename) {
			fprintf(stderr, "error: filename is mandatory\n");
			status = -1;
		} else {
			status += nxt_start_program(nxt, filename);
		}
	}

	if (stopflag) {
		status += nxt_stop_program(nxt);
	}

	nxt_close(nxt);
	return (status == 0) ? 0 : 1;
}
