IDIR =../../headers
CC=gcc
CFLAGS=-I$(IDIR) `pkg-config --cflags glib-2.0 gio-2.0 sqlite3` -Wall -Werror -g

ODIR=.
LDIR =../../lib

LIBS=`pkg-config --libs glib-2.0 gio-2.0 sqlite3`

_DEPS = lanio.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = SAP_discovery.o $(IDIR)/lanio.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

SAP_discovery: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(IDIR)/*.o
