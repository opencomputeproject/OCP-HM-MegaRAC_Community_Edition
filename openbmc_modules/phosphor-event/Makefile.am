AM_DEFAULT_SOURCE_EXT = .cpp

sbin_PROGRAMS = phosphor-eventd

phosphor_eventd_SOURCES = \
	event_messaged.cpp \
	message.cpp \
	event_messaged_sdbus.c
phosphor_eventd_LDFLAGS = $(SYSTEMD_LIBS)
phosphor_eventd_CFLAGS = $(SYSTEMD_CFLAGS)

SUBDIRS = test
