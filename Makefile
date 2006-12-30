TARGET = gcode2eps
SOURCES = $(wildcard *.c)
OBJECTS = $(subst .c$,.o,$(SOURCES))
HEADERS = $(wildcard *.h)
CC = gcc
LINK = gcc
STRIP = strip
LIBS = -lm

$(TARGET): .depend $(OBJECTS)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(LIBPATH) $(LIBS)
	$(STRIP) $(TARGET)

.c.o: $*.h common.h
	$(CC) -c $(CFLAGS) $(DEBUGFLAGS) $(INCPATH) -o $@ $<

-include .depend

version.h:
	echo "#define BUILD_DATE \"`/bin/date \"+%d %b %Y\"`\"" > version.h

clean:
	-rm -f .depend
	-rm -f $(OBJECTS)
	-rm -f *~ core *.core
	-rm -f version.h
	-rm -f $(TARGET)

depend:
.depend: Makefile $(SOURCES) $(HEADERS)
	@if [ ! -f .depend ]; then touch .depend; fi
	@makedepend -Y -f .depend  $(SOURCES) 2>/dev/null

test:
	echo $(OBJECTS)

dist:
	tar zcvf gcode2eps.tar.gz Makefile $(SOURCES) examples/*
