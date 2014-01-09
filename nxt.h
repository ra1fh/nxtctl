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

#ifndef NXT_H
#define NXT_H

#include "buf.h"

typedef struct {
	struct usb_device *dev;
	struct usb_dev_handle *handle;
	Buf *buf;
} NXT;


NXT* nxt_new(); 
int nxt_init(NXT *self);
int nxt_print_battery_level(NXT *self);
int nxt_print_firmware_version(NXT *self);
int nxt_print_device_info(NXT *self);
int nxt_print_files(NXT *self, const char *pattern);
int nxt_start_program(NXT *self, const char *filename);
int nxt_stop_program(NXT *self);
int nxt_get_file(NXT *self, const char *filename);
int nxt_put_file(NXT *self, const char *filename);
int nxt_delete_file(NXT *self, const char *filename);
int nxt_close(NXT *self);
int nxt_boot(NXT *self);
int nxt_print_infos();
int nxt_upload(char *fname);
int nxt_download(char *fname);

#endif
