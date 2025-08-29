VERSION := 0.1a
CC      := gcc
EXTRA_CFLAGS ?= 
EXTRA_LDLAGS ?= 
CFLAGS  := -Wall -Wno-unused-result -O3 $(EXTRA_CFLAGS)
LDFLAGS := -s $(EXTRA_LDFLAGS)
DESTDIR :=
PREFIX  := /usr
BINDIR  := /sbin
MANDIR  := /share/man
APPNAME := p53-fan
TARGET	:= $(APPNAME)
SOURCES := $(sort $(shell find src/ -type f -name *.c))
OBJECTS := $(patsubst src/%,build/%,$(SOURCES:.c=.o))
DEPS	:= $(OBJECTS:.o=.deps)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJECTS) 

build/%.o: src/%.c
	@mkdir -p build/
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -DAPPNAME=\"$(APPNAME)\" -MD -MF $(@:.o=.deps) -c -o $@ $< 

clean:
	$(RM) -r build/ $(TARGET) 

install:
	install -D -m 755 $(APPNAME) $(DESTDIR)/$(PREFIX)/$(BINDIR)/$(APPNAME)
	install -D -m 644 man1/* $(DESTDIR)/$(PREFIX)/$(MANDIR)/man1/

uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/$(BINDIR)/$(APPNAME)
	rm -f $(DESTDIR)/$(PREFIX)/$(MANDIR)/man1/$(APPNAME).1

-include $(DEPS)

.PHONY: clean install

