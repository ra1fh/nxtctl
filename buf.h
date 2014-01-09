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

#ifndef BUF_H
#define BUF_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
	unsigned char *buf;
	size_t size;
	size_t offset;
	size_t limit;
} Buf;

Buf* buf_new();
void buf_reset(Buf *self);
int buf_read_byte(Buf *self, uint8_t *d);
int buf_read_short(Buf *self, uint16_t *d);
int buf_read_uint(Buf *self, uint32_t *d);
int buf_read_string(Buf *self, char *s, size_t len);
int buf_read_data(Buf *self, char *s, size_t len);
int buf_read_skip(Buf *self, size_t len);
int buf_write_byte(Buf *self, uint8_t d);
int buf_write_short(Buf *self, uint16_t d);
int buf_write_uint(Buf *self, uint32_t d);
int buf_write_string(Buf *self, const char *s, size_t flen);
int buf_write_data(Buf *self, const char *s, size_t len);
int buf_pack(Buf *self, char *fmt, ...);
int buf_vpack(Buf *self, char *fmt, va_list ap);
int buf_unpack(Buf *self, char *fmt, ...);
int buf_vunpack(Buf *self, char *fmt, va_list ap);
int buf_check_limit(Buf *self);

#endif
