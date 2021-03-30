sbin_PROGRAMS = obmc-console-server
bin_PROGRAMS = obmc-console-client

obmcconsoledir = $(sysconfdir)/obmc-console
obmcconsole_DATA = conf/server.ttyVUART0.conf

if WITH_UDEV
udevrulesddir = $(udevdir)/rules.d
udevrulesd_DATA = conf/80-obmc-console-uart.rules
endif

if WITH_SYSTEMD
systemdsystemunit_DATA = conf/obmc-console@.service \
			 conf/obmc-console-ssh@.service

if ENABLE_CONCURRENT_SERVERS
obmcconsole_DATA += conf/client.2200.conf
else
systemdsystemunit_DATA += conf/obmc-console-ssh.socket
sshservicedir = $(systemdsystemunitdir)/obmc-console-ssh@.service.d
sshservice_DATA = conf/obmc-console-ssh@.service.d/use-socket.conf
endif
endif

obmc_console_server_CPPFLAGS = \
	-DLOCALSTATEDIR=\"$(localstatedir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\"

obmc_console_server_SOURCES = \
	 console-server.c \
	 console-server.h \
	 util.c \
	 ringbuffer.c \
	 config.c \
	 log-handler.c \
	 socket-handler.c \
	 tty-handler.c \
	 console-socket.c

obmc_console_server_LDFLAGS = $(SYSTEMD_LIBS) -lrt
obmc_console_server_CFLAGS = $(SYSTEMD_CFLAGS)

obmc_console_client_CPPFLAGS = \
	-DSYSCONFDIR=\"$(sysconfdir)\"

obmc_console_client_SOURCES = \
	 console-client.c \
	 console-server.h \
	 console-socket.c \
	 config.c \
	 util.c

SUBDIRS = test
