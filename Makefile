
.PHONY: clean

LDLIBS = -L/usr/local/lib -lusb-1.0
CFLAGS = -Wall -Werror -I/usr/local/include/libusb-1.0

PROG=		nxtctl
PREFIX?=	/usr/local

SRCS=		main.c nxt.c buf.c 
OBJS=		main.o nxt.o buf.o
HDRS=       nxt.h buf.h

CC =		gcc

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
