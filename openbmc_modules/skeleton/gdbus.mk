PACKAGE_DEPS=gio-unix-2.0 glib-2.0
LDLIBS+=-lopenbmc_intf

%.o: %_obj.c
	$(CC) -c $(ALL_CFLAGS) -o $@ $<
