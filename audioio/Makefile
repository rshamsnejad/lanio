IDIR =../headers
CC=gcc

ODIR=.
LDIR =../lib

CFLAGS=-I$(IDIR) -I$(LDIR) `pkg-config --cflags glib-2.0 gio-2.0 gstreamer-1.0 sqlite3` -Wall -Werror -g

LIBS=`pkg-config --libs glib-2.0 gio-2.0 gstreamer-1.0 sqlite3`

_DEPS = lanio.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ_receive = lanio-receive.o $(IDIR)/lanio.o $(LDIR)/fort.o
OBJ_receive = $(patsubst %,$(ODIR)/%,$(_OBJ_receive))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: lanio-receive

lanio-receive: $(OBJ_receive)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(IDIR)/*.o $(LDIR)/*.o
