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

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <usb.h>
#include "nxt.h"

/* USB IDs of a lego nxt brick */
#define LEGO_VENDOR_ID       0x0694
#define LEGO_NXT_PRODUCT_ID  0x0002

/* Some codes, defined in Appendix 1 of Bluetooth handbook */
#define NXT_DIRECT_COMMAND 0x00
#define NXT_SYSTEM_COMMAND 0x01
#define NXT_REPLY_COMMAND  0x02
#define NXT_DIRECT_COMMAND_NOREPLY 0x80
#define NXT_SYSTEM_COMMAND_NOREPLY 0x81

/* direct commands */
#define NXT_CMD_GET_BATTERY_LEVEL 0x0b
#define NXT_CMD_START_PROGRAM     0x00
#define NXT_CMD_STOP_PROGRAM      0x01


/* system commands */
#define NXT_CMD_OPEN_READ         	 0x80
#define NXT_CMD_OPEN_WRITE        	 0x81
#define NXT_CMD_READ              	 0x82
#define NXT_CMD_WRITE             	 0x83
#define NXT_CMD_CLOSE             	 0x84
#define NXT_CMD_DELETE            	 0x85
#define NXT_CMD_FIND_FIRST_FILE   	 0x86
#define NXT_CMD_FIND_NEXT_FILE    	 0x87
#define NXT_CMD_GET_FIRMWARE_VERSION 0x88
#define NXT_CMD_BOOT                 0x97
#define NXT_CMD_GET_DEVICE_INFO      0x9b

/* error codes */
#define NXT_SUCCESS                      0x00
#define NXT_ERROR_PENDING_TRANSACTION    0x20
#define NXT_ERROR_QUEUE_EMPTY            0x40
#define NXT_ERROR_NO_MORE_HANDLES        0x81
#define NXT_ERROR_NO_SPACE               0x82
#define NXT_ERROR_NO_MORE_FILES          0x83
#define NXT_ERROR_END_OF_FILE_EXPECTED   0x84
#define NXT_ERROR_END_OF_FILE            0x85
#define NXT_ERROR_NOT_A_LINEAR_FILE      0x86
#define NXT_ERROR_FILE_NOT_FOUND         0x87
#define NXT_ERROR_HANDLE_ALREADY_CLOSED  0x88
#define NXT_ERROR_NO_LINEAR_SPACE        0x89
#define NXT_ERROR_UNDEFINED_ERROR        0x8A
#define NXT_ERROR_FILE_IS_BUSY           0x8B
#define NXT_ERROR_NO_WRITE_BUFFERS       0x8C
#define NXT_ERROR_APPEND_NOT_POSSIBLE    0x8D
#define NXT_ERROR_FILE_IS_FULL           0x8E
#define NXT_ERROR_FILE_EXISTS            0x8F
#define NXT_ERROR_MODULE_NOT_FOUND       0x90
#define NXT_ERROR_OUT_OF_BOUNDARY        0x91
#define NXT_ERROR_ILLEGAL_FILE_NAME      0x92
#define NXT_ERROR_ILLEGAL_HANDLE         0x93
#define NXT_ERROR_REQUEST_FAILED         0xBD
#define NXT_ERROR_UNKNOWN_COMMAND_OPCODE 0xBE
#define NXT_ERROR_INSANE_PACKET          0xBF
#define NXT_ERROR_OUT_OF_RANGE           0xC0
#define NXT_ERROR_BUS_ERROR              0xDD
#define NXT_ERROR_COMM_OUT_OF_MEMORY     0xDE
#define NXT_ERROR_CHANNEL_INVALID        0xDF
#define NXT_ERROR_CHANNEL_BUSY           0xE0
#define NXT_ERROR_NO_ACTIVE_PROGRAM      0xEC
#define NXT_ERROR_ILLEGAL_SIZE           0xED
#define NXT_ERROR_ILLEGAL_QUEUE          0xEE
#define NXT_ERROR_INVALID_FIELD          0xEF
#define NXT_ERROR_BAD_INPUT_OUTPUT       0xF0
#define NXT_ERROR_INSUFFICIENT_MEMORY    0xFB
#define NXT_ERROR_BAD_ARGUMENTS          0xFF
		
#define NXT_WRITE_ENDPOINT 0x01
#define NXT_READ_ENDPOINT  0x82
#define NXT_WRITE_TIMEOUT  1000
#define NXT_READ_TIMEOUT   1000

#define USB_INTERFACE 0
#define USB_CONFIG 1

extern int vflag;

/***********************************************************************/
/* libusb helper functions                                             */
/***********************************************************************/
static struct usb_device* usb_find_usb_dev(int vendor, int product) {
	struct usb_bus *busses, *bus;
	struct usb_device *devs, *d;
	u_int16_t v, p;

	usb_find_busses();
	usb_find_devices();
	busses = usb_get_busses();
	for (bus = busses; bus;  bus = bus->next) {
		if (vflag)
			fprintf(stderr, "nxt_find_usb_dev: bus dirname=%s\n", bus->dirname);
		devs = bus->devices;
		for(d=devs; d; d=d->next){
			v = d->descriptor.idVendor;
			p = d->descriptor.idProduct;
			if (vflag)
				fprintf(stderr, "nxt_find_usb_dev: dev filename=%s, vendor=0x%04x, product=0x%04x\n",
						d->filename, v, p);
			if((v == vendor) && (p==product)) {
				if (vflag)
					fprintf(stderr, "nxt_find_usb_dev: found dev filename=%s, vendor=0x%04x, product=0x%04x\n",
							d->filename, v, p);
				return d;
			}
		}
	}
	return NULL;
}

static int usb_write(struct usb_dev_handle *handle, Buf *buf, const char *desc) {
	int len;
	if (vflag)
		printf("usb_write: offset=%zd\n", buf->offset);
	if ((len =usb_bulk_write(handle, NXT_WRITE_ENDPOINT, 
							 (char*) buf->buf, buf->offset, NXT_WRITE_TIMEOUT)) < 0) {
		fprintf(stderr, "usb_bulk_write failed for %s\n", desc);
		return -1;
	}
	return 0;
}

static int usb_read(struct usb_dev_handle *handle, Buf *buf, const char *desc) {
	int len;
	buf_reset(buf);
	if (vflag)
		printf("usb_read: offset=%zd size=%zd\n", buf->offset, buf->size);
	if ((len = usb_bulk_read(handle, NXT_READ_ENDPOINT, 
							 (char*) buf->buf, buf->size, NXT_READ_TIMEOUT)) < 0) {
		fprintf(stderr, "usb_bulk_read failed for %s (%d)\n", desc, len);
		return -1;
	}
	buf->limit = len;
	return 0;
}

static int usb_communicate(struct usb_dev_handle *handle, Buf *buf, const char*desc) {
	if (usb_write(handle, buf, desc) != 0) {
		return -1;
	}
	if (usb_read(handle, buf, desc) != 0) {
		return -1;
	}
	return 0;
}


/***********************************************************************/
/* nxt command wrappers                                                */
/***********************************************************************/
static const char* nxt_strerror(int error) {
	const char *str;

	switch(error) {
	case NXT_SUCCESS:
		str = "Success";
		break;
	case NXT_ERROR_PENDING_TRANSACTION:
		str = "Pending communication transaction in progress";
		break;
	case NXT_ERROR_QUEUE_EMPTY:
		str = "Specified mailbox queue is empty";
		break;
	case NXT_ERROR_NO_MORE_HANDLES:
		str = "No more handles";
		break;
	case NXT_ERROR_NO_SPACE:
		str = "No space";
		break;
	case NXT_ERROR_NO_MORE_FILES:
		str = "No more files";
		break;
	case NXT_ERROR_END_OF_FILE_EXPECTED:
		str = "End of file expected";
		break;
	case NXT_ERROR_END_OF_FILE:
		str = "End of file";
		break;
	case NXT_ERROR_NOT_A_LINEAR_FILE:
		str = "Not a linear file";
		break;
	case NXT_ERROR_FILE_NOT_FOUND:
		str = "File not found";
		break;
	case NXT_ERROR_HANDLE_ALREADY_CLOSED:
		str = "Handle all ready closed";
		break;
	case NXT_ERROR_NO_LINEAR_SPACE:
		str = "No linear space";
		break;
	case NXT_ERROR_UNDEFINED_ERROR:
		str = "Undefined error";
		break;
	case NXT_ERROR_FILE_IS_BUSY:
		str = "File is busy";
		break;
	case NXT_ERROR_NO_WRITE_BUFFERS:
		str = "No write buffers";
		break;
	case NXT_ERROR_APPEND_NOT_POSSIBLE:
		str = "Append not possible";
		break;
	case NXT_ERROR_FILE_IS_FULL:
		str = "File is full";
		break;
	case NXT_ERROR_FILE_EXISTS:
		str = "File exists";
		break;
	case NXT_ERROR_MODULE_NOT_FOUND:
		str = "Module not found";
		break;
	case NXT_ERROR_OUT_OF_BOUNDARY:
		str = "Out of boundary";
		break;
	case NXT_ERROR_ILLEGAL_FILE_NAME:
		str = "Illegal file name";
		break;
	case NXT_ERROR_ILLEGAL_HANDLE:
		str = "Illegal handle";
		break;
	case NXT_ERROR_REQUEST_FAILED:
		str = "Request failed (i.e. specified file not found)";
		break;
	case NXT_ERROR_UNKNOWN_COMMAND_OPCODE:
		str = "Unknown command opcode";
		break;
	case NXT_ERROR_INSANE_PACKET:
		str = "Insane packet";
		break;
	case NXT_ERROR_OUT_OF_RANGE:
		str = "Data contains out-of-range values";
		break;
	case NXT_ERROR_BUS_ERROR:
		str = "Communication bus error";
		break;
	case NXT_ERROR_COMM_OUT_OF_MEMORY:
		str = "No free memory in communication buffer";
		break;
	case NXT_ERROR_CHANNEL_INVALID:
		str = "Specified channel/connection is not valid";
		break;
	case NXT_ERROR_CHANNEL_BUSY:
		str = "Specified channel/connection not configured or busy";
		break;
	case NXT_ERROR_NO_ACTIVE_PROGRAM:
		str = "No active program";
		break;
	case NXT_ERROR_ILLEGAL_SIZE:
		str = "Illegal size specified";
		break;
	case NXT_ERROR_ILLEGAL_QUEUE:
		str = "Illegal mailbox queue ID specified";
		break;
	case NXT_ERROR_INVALID_FIELD:
		str = "Attempted to access invalid field of a structure";
		break;
	case NXT_ERROR_BAD_INPUT_OUTPUT:
		str = "Bad input or output specified";
		break;
	case NXT_ERROR_INSUFFICIENT_MEMORY:
		str = "Insufficient memory available";
		break;
	case NXT_ERROR_BAD_ARGUMENTS:
		str = "Bad arguments";
		break;
	default:
		str = "Unknown error";
	}
	return str;
}

static int nxt_failed(int status) {
	if (status == NXT_SUCCESS) {
		return 0;
	} else {
		fprintf(stderr, "error: %s (0x%x)\n", nxt_strerror(status), status);
		return 1;
	}
}

static int nxt_simple_command(NXT* self, char *desc, char *fmt, ...) {
	va_list ap;
	int ret;
	Buf *buf;
	unsigned char reply, command, status;

	buf = self->buf;
	buf_reset(buf);

	va_start(ap,fmt);
	ret = buf_vpack(buf, fmt, ap);
	va_end(ap);

	if (usb_communicate(self->handle, buf, desc) != 0)
		return -1;
	if (buf_unpack(buf, "bbb", &reply, &command, &status) == -1)
		return -1;
	if (nxt_failed(status))
		return -1;
	return 0;
}

static int nxt_cmd_write(NXT *self, 
						 unsigned char handle,
						 char *data,
						 unsigned short size) {
	Buf *buf = self->buf;
	unsigned short writesize;
	unsigned char writehandle;
	unsigned char reply, command, status;

	buf_reset(buf);

    /* send command */
	if (buf_pack(buf, "bbbd", NXT_SYSTEM_COMMAND, NXT_CMD_WRITE, handle, data, size) == -1)
		return -1;

	/* do usb transaction */
	if (usb_communicate(self->handle, buf, "WRITE") != 0)
		return -1;

	/* read result */
	if (buf_unpack(buf, "bbb", &reply, &command, &status) == -1)
		return -1;
	if (nxt_failed(status))
		return -1;
	if (buf_unpack(buf, "bh", &writehandle, &writesize) == -1)
		return -1;

	/* do some sanity checks */
	if (writesize != size) {
		fprintf(stderr, "error: writesize=%hu size=%hu\n", 
				writesize, size);
		return -1;
	}
	if (writehandle != handle) {
		fprintf(stderr, "error: handles don't match\n");
		return -1;
	}
	if (vflag) 
		fprintf(stderr, "nxt_cmd_write: data written: %hu\n", writesize);

	return 0;
}


static int nxt_cmd_read(NXT *self, unsigned char handle, char *data, unsigned short size) {
	Buf *buf = self->buf;
	unsigned short readsize;
	unsigned char readhandle;
	unsigned char reply, command, status;

	buf_reset(buf);

	if (buf_pack(buf, "bbbh", NXT_SYSTEM_COMMAND, NXT_CMD_READ, handle, size) == -1)
		return -1;
	
	/* do usb transaction */
	if (usb_communicate(self->handle, buf, "READ") != 0)
		return -1;
	
	/* read result */
	if (buf_unpack(buf, "bbb", &reply, &command, &status) == -1)
		return -1;
	if (nxt_failed(status))
		return -1;
	if (buf_unpack(buf, "bh", &readhandle, &readsize) == -1)
		return -1;
	
	/* do some sanity checks */
	if (readsize != size) {
		fprintf(stderr, "nxt_cmd_read: error: readsize=%hu size=%hu\n", 
				readsize, size);
		return -1;
	}
	
	/* read the actual data and write to outfile */
	buf_read_data(buf, data, size);
	if (vflag) 
		fprintf(stderr, "nxt_cmd_read: got data: %hu\n", readsize);

	return 0;
}

static int nxt_cmd_open_read(NXT *self, 
							 const char *filename, 
							 unsigned char *handle,
							 unsigned int  *filesize) {
	if (nxt_simple_command(self, "OPEN_READ", "bbs", 
						   NXT_SYSTEM_COMMAND, NXT_CMD_OPEN_READ, 
						   filename, 20) == -1)
		return -1;
	if (buf_unpack(self->buf, "bu", handle, filesize) == -1)
		return -1;
	return 0;
}

static int nxt_cmd_open_write(NXT *self, 
							  const char *filename, 
							  unsigned int  filesize,
							  unsigned char *handle) {
	if (nxt_simple_command(self, "OPEN_READ", "bbsu", 
						   NXT_SYSTEM_COMMAND, NXT_CMD_OPEN_WRITE, 
						   filename, 20, filesize) == -1)
		return -1;
	if (buf_read_byte(self->buf, handle) == -1)
		return -1;
	return 0;
}

static int nxt_cmd_close(NXT *self, unsigned char handle) {
	return nxt_simple_command(self, "CLOSE", "bbb", 
							  NXT_SYSTEM_COMMAND, NXT_CMD_CLOSE, handle);
}

static int nxt_cmd_delete(NXT *self, const char* filename) {
	return nxt_simple_command(self, "DELETE", "bbs", 
							  NXT_SYSTEM_COMMAND, NXT_CMD_DELETE, filename, 20);
}

/*
 * pattern:  if not null, find first file matching this pattern. if
 *           null, find next file.
 * handle:   on find first returns the handle, on find next has to be a
 *           valid handle
 * filename: return the found file. should have space for at least 20 characters
 * filesize: returns the size of the file
 *
 * returns 0 on success and -1 on error and -2 on file not found
 */
static int nxt_cmd_find(NXT *self, 
						const char *pattern, 
						unsigned char *handle, 
						char *filename, 
						unsigned int *filesize) {
	Buf *buf = self->buf;
	unsigned char reply, command, status;

	buf_reset(buf);
	buf_write_byte(buf, NXT_SYSTEM_COMMAND);
	if (pattern) {
		buf_write_byte(buf, NXT_CMD_FIND_FIRST_FILE);
		buf_write_string(buf, pattern, 20);
	} else {
		buf_write_byte(buf, NXT_CMD_FIND_NEXT_FILE);
		buf_write_byte(buf, *handle);
	}
	
	if (usb_communicate(self->handle, buf, "find first/next file") != 0)
		return -1;

	if (buf_unpack(buf, "bbb", &reply, &command, &status) == -1)
		return -1;

	if (status == NXT_ERROR_FILE_NOT_FOUND)
		return -2;
	if (nxt_failed(status))
		return -1;

	if (buf_read_byte(buf, handle) == -1)
		return -1;

	if (filename) {
		buf_read_string(buf, filename, 20);
	} else {
		buf_read_skip(buf, 20);
	}

	if (filesize) {
		buf_read_uint(buf, filesize);
	}

	return 0;
}

static int nxt_cmd_boot(NXT *self) {
	const char *samba = "Let's dance: SAMBA";
	return nxt_simple_command(self, "BOOT", "bbd", 
							  NXT_SYSTEM_COMMAND, NXT_CMD_BOOT, 
							  samba, strlen(samba) + 1);
}

/*************************************************************/
/* nxt class */
/*************************************************************/
NXT* nxt_new() {
	NXT* res;
	if ((res = (NXT*) malloc(sizeof(NXT))) == NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}
	res->buf = buf_new();
	return res;
}

int nxt_init(NXT *self) {
	unsigned int vendor = LEGO_VENDOR_ID;
	unsigned int product = LEGO_NXT_PRODUCT_ID;

	usb_init();
	self->dev = usb_find_usb_dev(vendor, product);
	if (self->dev == NULL) {
		fprintf(stderr, "no usb device found (vendor=0x%04x product=0x%04x)\n",
				vendor, product);
		return -1;
	}

	self->handle = usb_open(self->dev);
	if (! self->handle) {
		fprintf(stderr, "failed to open device handle (vendor=0x%04x product=0x%04x)",
				vendor, product);
		return -1;
	}
	usb_reset(self->handle);

	int err;
	err = usb_set_configuration(self->handle, USB_CONFIG);
	if(err != 0){
		fprintf(stderr, "fails to set config, errno=%d "
				        "(cf=%d vendor=0x%04x product=0x%04x)",
				err, USB_CONFIG, vendor, product);
		return -1;
	}

	err = usb_claim_interface(self->handle, USB_INTERFACE);
	if(err != 0){
		fprintf(stderr, "fails to optain usb interface "
				        "(erno=%d id=%d vendor=0x%04x product=0x%04x)",
				err, USB_INTERFACE, vendor, product);
		return -1;
	}
	return 0;
}

int nxt_close(NXT *self) {
	return usb_close(self->handle);
}


int nxt_print_battery_level(NXT* self){
	unsigned short mv;

	if (nxt_simple_command(self, "GET_BATTERY_LEVEL", "bb",
						   NXT_DIRECT_COMMAND, NXT_CMD_GET_BATTERY_LEVEL) == -1)
		return -1;
	if (buf_read_short(self->buf, &mv) == -1) 
		return -1;

	printf("battery level: %dmV\n", mv);
	return 0;
}

int nxt_print_firmware_version(NXT* self){
	unsigned char pmajor, pminor;
	unsigned char fmajor, fminor;

	if (nxt_simple_command(self, "GET_FIRMWARE_VERSION", "bb", 
						   NXT_SYSTEM_COMMAND, NXT_CMD_GET_FIRMWARE_VERSION) == -1)
		return -1;

	if (buf_unpack(self->buf, "bbbb", &pminor, &pmajor, &fminor, &fmajor) == -1)
		return -1;

	printf("protocol version: %hhu.%hhu\n", pmajor, pminor);
	printf("firmware version: %hhu.%hhu\n", fmajor, fminor);

	return 0;
}

int nxt_print_device_info(NXT* self){
	int i;
	unsigned int signal_strength;
	unsigned int free_space;
	unsigned char btaddr[7];
	char name[15];

	if (nxt_simple_command(self, "GET_DEVICE_INFO", "bb",
						   NXT_SYSTEM_COMMAND, NXT_CMD_GET_DEVICE_INFO) == -1)
		return -1;

	buf_read_string(self->buf, name, 15);
	for (i = 0; i < sizeof(btaddr); ++i)
		buf_read_byte(self->buf, &btaddr[i]);
	buf_read_uint(self->buf, &signal_strength);
	buf_read_uint(self->buf, &free_space);

	printf("nxt name: %s\n", name);
	printf("bluetooth address: ");
	for (i=0; i < sizeof(btaddr); ++i) {
		if (i > 0)
			printf(":");
		printf("%02hhx", btaddr[i]);
	}
	printf("\n");
	printf("bluetooth signal strength: %u\n", signal_strength);
	printf("free user flash: %u\n", free_space);

	return 0;
}

int nxt_print_files(NXT *self, const char *pattern) {
	char filename[20];
	Buf *buf;
	int error = 0;
	int counter = 0;
	unsigned int filesize;
	unsigned char handle;
	unsigned char handle_valid = 0;

	buf = self->buf;
	counter = 0;

	if (pattern && strlen(pattern) >= 20) {
		fprintf(stderr, "error: pattern too long\n");
		return -1;
	}

	if (! pattern) {
		pattern = "*.*";
	}

	while (1) {
		int res;
		res = nxt_cmd_find(self, pattern, &handle, filename, &filesize);
		/* file not found */
		if (res == -2) {
			break;
		}
		if (res != 0) {
			error = 1;
			break;
		}

		/* this makes nxt_cmd_find do "find next" instead of "find first" */
		pattern = 0;
		handle_valid = 1;
		counter++;
		printf("%6u %s\n", filesize, filename);
	}

	if (handle_valid) {
		if (nxt_cmd_close(self, handle) != 0) {
			error = 1;
		}
	}
	
	return (error ? -1 : 0);
}

int nxt_start_program(NXT* self, const char* filename){
	Buf *buf;
	unsigned char reply, command, status;

	if (!filename || strlen(filename) >= 20) {
		fprintf(stderr, "error: filename missing or too long\n");
		return -1;
	}

	buf = self->buf;
	buf_reset(buf);
	buf_pack(buf, "bbs", NXT_DIRECT_COMMAND, NXT_CMD_START_PROGRAM, filename, 20);

	if (usb_communicate(self->handle, buf, "START_PROGRAM") != 0) {
		return -1;
	}

	buf_unpack(buf, "bbb", &reply, &command, &status);

	if (status == NXT_ERROR_OUT_OF_RANGE) {
		/* seems to be error for file not found */
		fprintf(stderr, "error: file not found\n");
		return -1;
	}

	if (nxt_failed(status)) {
			return -1;
	}

	return 0;
}

int nxt_stop_program(NXT* self){
	return nxt_simple_command(self, "STOP_PROGRAM", "bb", NXT_DIRECT_COMMAND, NXT_CMD_STOP_PROGRAM);
}

/* max chunk size (64) - header (6) - one byte too much??? (1) */
#define NXT_READ_SIZE 57 

int nxt_get_file_fd(NXT *self, const char *filename, int fd) {
	char data[BUFSIZ];
	unsigned int filesize;
	unsigned short chunksize;
	unsigned int transferred = 0;
	unsigned char handle;

	/* open file handle */
	if (nxt_cmd_open_read(self, filename, &handle, &filesize) != 0) {
		return -1;
	}

	/* read data */
	while (filesize > 0) {
		/* build command */
		if (filesize >= NXT_READ_SIZE)
			chunksize = NXT_READ_SIZE;
		else
			chunksize = filesize;

		if (nxt_cmd_read(self, handle, data, chunksize) != 0) {
			break;
		}

		if (vflag) 
			fprintf(stderr, "nxt_get_file_fd: filesize=%d, chunksize=%d\n", filesize, chunksize);

		write(fd, data, chunksize);
		transferred += chunksize;
		filesize -= chunksize;
	}

	if (nxt_cmd_close(self, handle) != 0) {
		return -1;
	}

	if (filesize == 0) {
		printf("%u bytes transfered to %s\n", transferred, filename);
		return 0;
	} else {
		return -1;
	}
}

int nxt_get_file(NXT *self, const char *filename) {
	int res;
	int fd;

	if (!filename) {
		fprintf(stderr, "error: filename missing\n");
		return -1;
	}
	if (strlen(filename) >= 20) {
		fprintf(stderr, "error: filename too long\n");
		return -1;
	}

	fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0777);
	if (fd < 0) {
		fprintf(stderr, "error: could not open local file %s\n", filename);
		return -1;
	}
	res = nxt_get_file_fd(self, filename, fd);
	close(fd);

	return res;
}

/* max chunk size (64) - header (3) - one byte too much??? (1) */
#define NXT_WRITE_SIZE 60 

static int nxt_put_file_fd(NXT* self, const char *filename, int fd) {
	char data[BUFSIZ];
	struct stat sb;
	ssize_t nr;
	unsigned int filesize;
	unsigned short chunksize;
	unsigned short byteswritten = 0;
	unsigned char handle;

	if (fstat(fd, &sb) != 0) {
		fprintf(stderr, "error: could not get file size %s\n", filename);
		return -1;
	}
	filesize = sb.st_size;

	if (nxt_cmd_find(self, filename, &handle, 0, 0) == 0) {
		nxt_cmd_close(self, handle);
		nxt_cmd_delete(self, filename);
	}

	if (nxt_cmd_open_write(self, filename, filesize, &handle) != 0) {
		return -1;
	}

	while (filesize > 0) {
		if (filesize >= NXT_WRITE_SIZE)
			chunksize = NXT_WRITE_SIZE;
		else
			chunksize = filesize;

		if ((nr = read(fd, data, chunksize)) != chunksize) {
			fprintf(stderr, "nxt_put_file: read failed. chunksize=%hd, nr=%zd\n", chunksize, nr);
			break;
		}
		if (nxt_cmd_write(self, handle, data, chunksize) != 0) {
			break;
		}
		if (vflag) 
			fprintf(stderr, "nxt_put_file: filesize=%u, chunksize=%hd\n", filesize, chunksize);
		byteswritten += chunksize;
		filesize -= chunksize;
	}

	if (nxt_cmd_close(self, handle) != 0) {
		return -1;
	}

	printf("%hu bytes uploaded to %s\n", byteswritten, filename);
	return 0;
}

int nxt_put_file(NXT* self, const char* filename){
	int res;
	int fd;

	if (!filename) {
		fprintf(stderr, "error: filename missing\n");
		return -1;
	}
	if (strlen(filename) >= 20) {
		fprintf(stderr, "error: filename too long\n");
		return -1;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error: could not open local file %s\n", filename);
		return -1;
	}
	res = nxt_put_file_fd(self, filename, fd);
	close(fd);

	return res;
}

int nxt_delete_file(NXT* self, const char* filename){
	int res;

	res = nxt_cmd_delete(self, filename);
	if (res != 0) {
		fprintf(stderr, "error: could delete remote file %s\n", filename);
	} else {
		fprintf(stderr, "file deleted: %s\n", filename);
	}

	return res;
}

int nxt_boot(NXT* self) {
	int res;

	res = nxt_cmd_boot(self);
	if (res != 0) {
		fprintf(stderr, "error: boot failed\n");
	}

	return res;
}
