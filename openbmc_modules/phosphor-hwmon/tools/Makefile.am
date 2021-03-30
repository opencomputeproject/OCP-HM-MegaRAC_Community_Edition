noinst_PROGRAMS = find_callout_path find_hwmon

find_callout_path_SOURCES = find_callout_path.cpp
find_callout_path_LDFLAGS = -static
find_callout_path_LDADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${top_builddir}/sysfs.o
find_callout_path_CXXFLAGS =

find_hwmon_SOURCES = find_hwmon.cpp
find_hwmon_LDFLAGS = -static
find_hwmon_LDADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	${top_builddir}/sysfs.o
find_hwmon_CXXFLAGS =
