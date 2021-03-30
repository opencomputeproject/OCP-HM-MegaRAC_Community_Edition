libdir=/usr/lib
includedir=/usr/include

PACKAGE_DEPS=gio-unix-2.0 glib-2.0
SONAME=libopenbmc_intf.so
VERSION=1
LIBOBMC=$(SONAME).$(VERSION)
INCLUDES=openbmc_intf.h openbmc.h gpio.h gpio_configs.h

LDLIBS+=$(shell pkg-config --libs $(PACKAGE_DEPS)) -lcjson
ALL_CFLAGS+=$(shell pkg-config --cflags $(PACKAGE_DEPS)) -fPIC -Werror $(CFLAGS)

all: $(SONAME)

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

$(SONAME): $(LIBOBMC)
	ln -sf $^ $@

$(LIBOBMC): lib%.so.$(VERSION): %.o gpio.o gpio_configs.o gpio_json.o
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -Wl,-soname,$(SONAME) \
		-o $@ $^ $(LDLIBS)

install: $(SONAME) $(LIBOBMC)
	@mkdir -p $(DESTDIR)$(includedir)
	install $(INCLUDES) $(DESTDIR)$(includedir)
	@mkdir -p $(DESTDIR)$(libdir)
	install $(LIBOBMC) $(DESTDIR)$(libdir)
	ln -sf $(LIBOBMC) $(DESTDIR)$(libdir)/$(SONAME)

clean:
	rm -f *.o $(LIBOBMC) $(SONAME)
