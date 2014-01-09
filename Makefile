
.PHONY: clean

LDLIBS = -L/usr/local/lib -lusb
CFLAGS = -Wall -Werror -I/usr/local/include

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
