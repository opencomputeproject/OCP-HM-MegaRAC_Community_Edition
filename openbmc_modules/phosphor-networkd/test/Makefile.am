AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir}

TESTS = $(check_PROGRAMS)

check_PROGRAMS = test

test_SOURCES = \
	test_util.cpp \
	mock_syscall.cpp \
	test_neighbor.cpp \
	test_netlink.cpp \
	test_network_manager.cpp \
	test_ethernet_interface.cpp \
	test_rtnetlink.cpp \
	test_config_parser.cpp \
	test_vlan_interface.cpp

generic_cpp_flags = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS)

generic_cxx_flags = $(PTHREAD_CFLAGS) \
					$(SYSTEMD_CFLAGS) \
					$(SDBUSPLUS_CFLAGS) \
					$(SDEVENTPLUS_CFLAGS) \
					$(STDPLUS_CFLAGS) \
					$(PHOSPHOR_LOGGING_CFLAGS) \
					$(PHOSPHOR_DBUS_INTERFACES_CFLAGS)

generic_ld_flags =  -lgtest_main -lgtest -lgmock \
					$(OESDK_TESTCASE_FLAGS) \
					$(SYSTEMD_LIBS) \
					$(SDBUSPLUS_LIBS) \
					$(SDEVENTPLUS_LIBS) \
					$(STDPLUS_LIBS) \
					$(PHOSPHOR_LOGGING_LIBS) \
					$(PHOSPHOR_DBUS_INTERFACES_LIBS)

test_CPPFLAGS = ${generic_cpp_flags}
test_CXXFLAGS = ${generic_cxx_flags}
test_LDFLAGS  = ${generic_ld_flags} -ldl

test_LDADD = $(top_builddir)/ethernet_interface.o \
			$(top_builddir)/network_manager.o \
			$(top_builddir)/network_config.o \
			$(top_builddir)/ipaddress.o \
			$(top_builddir)/neighbor.o \
			$(top_builddir)/netlink.o \
			$(top_builddir)/routing_table.o \
			$(top_builddir)/util.o \
			$(top_builddir)/rtnetlink_server.o \
			$(top_builddir)/system_configuration.o \
			$(top_builddir)/dhcp_configuration.o \
			$(top_builddir)/config_parser.o \
			$(top_builddir)/vlan_interface.o \
			$(top_builddir)/xyz/openbmc_project/Network/VLAN/Create/phosphor_network_manager-server.o \
			$(top_builddir)/xyz/openbmc_project/Network/IP/Create/phosphor_network_manager-server.o \
			$(top_builddir)/xyz/openbmc_project/Network/Neighbor/CreateStatic/phosphor_network_manager-server.o
