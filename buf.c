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

#include <arpa/inet.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"

extern int vflag;

/*************************************************************/
/* buf class */
/*************************************************************/
Buf* buf_new() {
	Buf* res;
	if ((res = (Buf*) malloc(sizeof(Buf))) == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	if ((res->buf = (unsigned char *) malloc(BUFSIZ)) == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	res->size = BUFSIZ;
	res->offset = 0;
	res->limit  = 0;
	
	return res;
}

void buf_reset(Buf *self) {
	self->offset = 0;
	self->limit  = 0;
}

int buf_read_byte(Buf *self, uint8_t *d) {
	if (self->offset + sizeof(*d) >= self->size) {
		return -1;
	}
	if (vflag)
		printf("buf_read_byte: offset=%zd, byte: %hhx\n", 
			   self->offset, self->buf[self->offset]);
	*d = self->buf[self->offset++];
	return sizeof(*d);
}

int buf_read_short(Buf *self, uint16_t *d) {
	uint16_t result = 0;
	
	if (self->offset + sizeof(*d) >= self->size) {
		return -1;
	}
	
	/* convert little endian to native byte order */
	result |= self->buf[self->offset++];
	result |= self->buf[self->offset++] << 8;

	*d = result;
	return sizeof(*d);
}

int buf_read_uint(Buf *self, uint32_t *d) {
	uint32_t result = 0;

	if (self->offset + sizeof(*d) >= self->size) {
		return -1;
	}

	/* convert little endian to native byte order */
	result |= self->buf[self->offset++];
	result |= self->buf[self->offset++] << 8;
	result |= self->buf[self->offset++] << 16;
	result |= self->buf[self->offset++] << 24;

	*d = result;
	return sizeof(*d);
}

int buf_read_string(Buf *self, char *s, size_t len) {
	size_t i;
	char c;
	if (self->offset + len >= self->size) {
		return -1;
	}
	i = 0;
	while (i < len && (c = self->buf[self->offset])) {
		if (vflag)
			printf("buf_read_string: offset=%zd, i=%zd, char=%c\n", self->offset, i, self->buf[self->offset]);
		s[i] = c;
		i++;
		self->offset++;
	}
	/* fill with 0 bytes up to len */
	while (i < len) {
		if (vflag)
			printf("buf_read_string: offset=%zd, i=%zd, char=%c\n", self->offset, i, 0);
		s[i++] = 0;
		self->offset++;
	}
	return 0;
}

int buf_read_data(Buf *self, char *s, size_t len) {
	size_t i;
	if (self->offset + len >= self->size) {
		return -1;
	}
	i = 0;
	while (i < len) {
		if (vflag > 1)
			printf("buf_read_data: offset=%zd, i=%zd, data=0x%02hhx\n", self->offset, i, self->buf[self->offset]);
		s[i++] = self->buf[self->offset++];
	}
	return 0;
}

int buf_read_skip(Buf *self, size_t skip) {
	if (self->offset + skip < self->size) {
		self->offset += skip;
		return 0;
	} else {
		return -1;
	}
}

int buf_write_byte(Buf *self, uint8_t d) {
	if (self->offset + sizeof(d) >= self->size) {
		return -1;
	}
	if (vflag)
		printf("buf_write_byte offset=%zd, byte: %hhx\n", 
			   self->offset, d);
	self->buf[self->offset++] = d;
	return sizeof(d);
}

int buf_write_short(Buf *self, uint16_t d) {
	if (self->offset + sizeof(d) >= self->size) {
		return -1;
	}
	/* write single bytes to get little-endian */
	self->buf[self->offset++] = d;
	self->buf[self->offset++] = d >> 8;
	return sizeof(d);
}

int buf_write_uint(Buf *self, uint32_t d) {
	if (self->offset + sizeof(d) >= self->size) {
		return -1;
	}
	/* write single bytes to get little-endian */
	self->buf[self->offset++] = d;
	self->buf[self->offset++] = d >> 8;
	self->buf[self->offset++] = d >> 16;
	self->buf[self->offset++] = d >> 24;
	return sizeof(d);
}

/*
 * Write zero terminated string and zero pad up to flen
 */
int buf_write_string(Buf *self, const char *s, size_t flen) {
	size_t slen = strlen(s);
	size_t i;
	if (vflag)
		printf("buf_write_string: s=%s, len=%zd\n", s, flen);
	if (self->offset + slen + 1 < self->size && slen < flen - 1) {
		i = 0;
		while(s[i] && i < flen - 1) {
			if (vflag)
				printf("buf_write_string: offset=%zd, i=%zd, char=%c\n", self->offset, i, s[i]);
			self->buf[self->offset++] = s[i++];
		}
		while(i < flen) {
			if (vflag)
				printf("buf_write_string: offset=%zd, i=%zd, char=%c\n", self->offset, i, 0);
			self->buf[self->offset++] = 0;
			i++;
		}
	} else {
		return -1;
	}
	return 0;
}

/*
 * Write data wite data with len
 */
int buf_write_data(Buf *self, const char *s, size_t len) {
	size_t i;
	if (self->offset + len >= self->size) {
		return -1;
	}
	i = 0;
	while (i < len) {
		if (vflag > 1)
			printf("buf_write_data: offset=%zd, i=%zd, data=0x%02hhx\n", self->offset, i, s[i]);
		self->buf[self->offset++] = s[i++];
	}
	return 0;
}

int buf_pack(Buf *self, char *fmt, ...) {
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = buf_vpack(self, fmt, ap);
	va_end(ap);
	return (ret);
}

int buf_vpack(Buf *self, char *fmt, va_list ap) {
	char *s;
	char *p;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	size_t len;
	int res;
	int bytes = 0;

	for (p = fmt; *p != '\0'; p++) {
		switch(*p) {
		case 'b': /* char */
			u8 = va_arg(ap, int);
			res = buf_write_byte(self, u8);
			break;
		case 'h': /* short */
			u16 = va_arg(ap, int);
			res = buf_write_short(self, u16);
			break;
		case 'u': /* uint */
			u32 = va_arg(ap, uint32_t);
			res = buf_write_uint(self, u32);
			break;
		case 's': /* string */
			s = va_arg(ap, char*);
			len = va_arg(ap, size_t);
			res = buf_write_string(self, s, len);
			break;
		case 'd': /* data */
			s = va_arg(ap, char*);
			len = va_arg(ap, size_t);
			res = buf_write_data(self, s, len);
			break;
		default: /* illegal format */
			res = -1;
		}
		if (res == -1) {
			return -1;
		}
		bytes += res;
	}
	return bytes;
}

int buf_vunpack(Buf *self, char *fmt, va_list ap) {
	char *s;
	char *p;
	uint8_t *u8p;
	uint16_t *u16p;
	uint32_t *u32p;
	size_t len;
	int res;
	int bytes = 0;

	for (p = fmt; *p != '\0'; p++) {
		switch(*p) {
		case 'b': /* char */
			u8p = va_arg(ap, uint8_t*);
			res = buf_read_byte(self, u8p);
			break;
		case 'h': /* short */
			u16p = va_arg(ap, uint16_t*);
			res = buf_read_short(self, u16p);
			break;
		case 'u': /* uint */
			u32p = va_arg(ap, uint32_t*);
			res = buf_read_uint(self, u32p);
			break;
		case 's': /* string */
			s = va_arg(ap, char*);
			len = va_arg(ap, size_t);
			res = buf_read_string(self, s, len);
			break;
		default: /* illegal format */
			res = -1;
		}
		if (res == -1) {
			return -1;
		}
		bytes += res;
	}
	return bytes;
}

int buf_unpack(Buf *self, char *fmt, ...) {
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = buf_vunpack(self, fmt, ap);
	va_end(ap);
	return (ret);
}

int buf_check_limit(Buf *self) {
	if (vflag)
		printf("buf_check_limit: offset=%zd, limit=%zd\n", self->offset, self->limit);
	if (self->offset == self->limit) {
		return 0;
	} else {
		return -1;
	}
}
