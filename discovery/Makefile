IDIR =../headers
CC=gcc
CFLAGS=-I$(IDIR) `pkg-config --cflags glib-2.0 gio-2.0 sqlite3` -Wall -Werror -g

ODIR=.
LDIR =../lib

LIBS=`pkg-config --libs glib-2.0 gio-2.0 sqlite3`

_DEPS = lanio.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ_discovery = lanio-discovery.o $(IDIR)/lanio.o
OBJ_discovery = $(patsubst %,$(ODIR)/%,$(_OBJ_discovery))

_OBJ_list-discovered = lanio-list-discovered.o $(IDIR)/lanio.o
OBJ_list-discovered = $(patsubst %,$(ODIR)/%,$(_OBJ_list-discovered))

_OBJ_parsesdp = parsesdp.o $(IDIR)/lanio.o
OBJ_parsesdp = $(patsubst %,$(ODIR)/%,$(_OBJ_parsesdp))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: lanio-discovery lanio-list-discovered parsesdp

lanio-discovery: $(OBJ_discovery)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
lanio-list-discovered: $(OBJ_list-discovered)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
parsesdp: $(OBJ_parsesdp)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(IDIR)/*.o
