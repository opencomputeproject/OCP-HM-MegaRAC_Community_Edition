COMMON_CXX = \
	-flto \
	-Wno-psabi \
	$(SYSTEMD_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	-DBOOST_ERROR_CODE_HEADER_ONLY \
	-DBOOST_SYSTEM_NO_DEPRECATED \
	-DBOOST_COROUTINES_NO_DEPRECATION_WARNING \
	-DBOOST_ASIO_DISABLE_THREADS \
	-DBOOST_ALL_NO_LIB

pkgconfig_DATA = libipmid.pc
lib_LTLIBRARIES = libipmid.la
libipmid_la_SOURCES = \
	sdbus-asio.cpp \
	signals.cpp \
	systemintf-sdbus.cpp \
	utils.cpp
libipmid_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	-version-info 0:0:0 -shared
libipmid_la_CXXFLAGS = \
	$(COMMON_CXX)
