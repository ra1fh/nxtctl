
.PHONY: clean

LIBUSB_CFLAGS != pkg-config --cflags libusb
LIBUSB_LIBS   != pkg-config --libs libusb

LDLIBS = $(LIBUSB_LIBS)
CFLAGS = -Wall -Werror $(LIBUSB_CFLAGS) 

PROG=		nxtctl
PREFIX?=	/usr/local

SRCS=		main.c nxt.c buf.c 
OBJS=		main.o nxt.o buf.o
HDRS=       nxt.h buf.h

CC =		scan-build clang

INSTALLDIR=	install -d
INSTALLBIN= install -m 0555

.SUFFIXES: .c .o

all: $(PROG)

$(OBJS): $(HDRS)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) $(PROG)

install: $(PROG)
	$(INSTALLDIR) $(DESTDIR)$(PREFIX)/bin
	$(INSTALLBIN) $(PROG) $(DESTDIR)$(PREFIX)/bin
