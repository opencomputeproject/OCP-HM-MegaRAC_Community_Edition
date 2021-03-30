AM_DEFAULT_SOURCE_EXT = .cpp

libexec_PROGRAMS = \
	chassiskill

chassiskill_SOURCES = \
	chassiskill.cpp

chassiskill_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(GPIOPLUS_FLAGS)

chassiskill_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(GPIOPLUS_LIBS)
