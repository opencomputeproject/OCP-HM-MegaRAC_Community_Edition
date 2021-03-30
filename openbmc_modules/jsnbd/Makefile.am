sbin_PROGRAMS = nbd-proxy

nbd_proxy_CPPFLAGS = \
	$(JSON_CFLAGS) \
	$(UDEV_CFLAGS) \
	-DRUNSTATEDIR=\"$(localstatedir)/run\" \
	-DSYSCONFDIR=\"$(sysconfdir)\"

nbd_proxy_LDADD = \
	$(JSON_LIBS) \
	$(UDEV_LIBS)
