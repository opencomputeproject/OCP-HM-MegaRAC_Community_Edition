AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -I$(top_srcdir)

noinst_HEADERS = fru-fault-monitor.hpp

bin_PROGRAMS = phosphor-fru-fault-monitor

phosphor_fru_fault_monitor_SOURCES = \
               fru-fault-monitor.cpp \
               monitor-main.cpp \
               xyz/openbmc_project/Led/Mapper/error.cpp \
               xyz/openbmc_project/Led/Fru/Monitor/error.cpp

BUILT_SOURCES =  xyz/openbmc_project/Led/Mapper/error.hpp \
                 xyz/openbmc_project/Led/Mapper/error.cpp \
                 xyz/openbmc_project/Led/Fru/Monitor/error.hpp \
                 xyz/openbmc_project/Led/Fru/Monitor/error.cpp

CLEANFILES = ${BUILT_SOURCES}

phosphor_fru_fault_monitor_LDFLAGS = $(SDBUSPLUS_LIBS) \
                                     $(PHOSPHOR_LOGGING_LIBS) \
                                     $(PHOSPHOR_DBUS_INTERFACES_LIBS)
phosphor_fru_fault_monitor_CFLAGS =  $(SDBUSPLUS_CFLAGS) \
                                     $(PHOSPHOR_LOGGING_CFLAGS) \
                                     $(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

xyz/openbmc_project/Led/Mapper/error.hpp: ${top_srcdir}/xyz/openbmc_project/Led/Mapper.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-header xyz.openbmc_project.Led.Mapper > $@

xyz/openbmc_project/Led/Mapper/error.cpp: ${top_srcdir}/xyz/openbmc_project/Led/Mapper.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-cpp xyz.openbmc_project.Led.Mapper > $@

xyz/openbmc_project/Led/Fru/Monitor/error.hpp: ${top_srcdir}/xyz/openbmc_project/Led/Fru/Monitor.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-header xyz.openbmc_project.Led.Fru.Monitor > $@

xyz/openbmc_project/Led/Fru/Monitor/error.cpp: ${top_srcdir}/xyz/openbmc_project/Led/Fru/Monitor.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(top_srcdir) error exception-cpp xyz.openbmc_project.Led.Fru.Monitor > $@
