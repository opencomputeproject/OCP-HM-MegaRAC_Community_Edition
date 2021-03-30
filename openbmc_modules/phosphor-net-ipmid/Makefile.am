BOOST_CXX = \
	-DBOOST_ERROR_CODE_HEADER_ONLY \
	-DBOOST_SYSTEM_NO_DEPRECATED \
	-DBOOST_COROUTINES_NO_DEPRECATION_WARNING \
	-DBOOST_ASIO_DISABLE_THREADS \
	-DBOOST_ALL_NO_LIB

bin_PROGRAMS = \
	netipmid

netipmid_SOURCES = \
	endian.hpp \
	socket_channel.hpp \
	message.hpp \
	auth_algo.hpp \
	auth_algo.cpp \
	session.hpp \
	sessions_manager.hpp \
	sessions_manager.cpp \
	message_parsers.hpp \
	message_parsers.cpp \
	message_handler.hpp \
	message_handler.cpp \
	command_table.hpp \
	command_table.cpp \
	command/channel_auth.hpp \
	command/channel_auth.cpp \
	command/guid.hpp \
	command/guid.cpp \
	command/open_session.hpp \
	command/open_session.cpp \
	command/rakp12.hpp \
	command/rakp12.cpp \
	command/rakp34.hpp \
	command/rakp34.cpp \
	command/session_cmds.hpp \
	command/session_cmds.cpp \
	comm_module.hpp \
	comm_module.cpp \
	main.hpp \
	main.cpp \
	integrity_algo.hpp \
	integrity_algo.cpp \
	crypt_algo.hpp \
	crypt_algo.cpp \
	sol/console_buffer.hpp \
	sd_event_loop.hpp \
	sol/sol_context.hpp \
	sol/sol_manager.hpp \
	sd_event_loop.cpp \
	sol/sol_manager.cpp \
	sol/sol_context.cpp \
	command/sol_cmds.hpp \
	command/sol_cmds.cpp \
	command/payload_cmds.hpp \
	command/payload_cmds.cpp \
	sol_module.hpp \
	sol_module.cpp

netipmid_CPPFLAGS = -DNET_IPMID_LIB_PATH=\"/usr/lib/net-ipmid/\"


netipmid_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(CRYPTO_LIBS) \
	$(libmapper_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(LIBIPMID_LIBS) \
	-lboost_coroutine \
	-lchannellayer \
	-luserlayer

netipmid_CXXFLAGS = \
	-flto \
	$(SYSTEMD_CFLAGS) \
	$(BOOST_CXX) \
	$(libmapper_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(LIBIPMID_CFLAGS)

if HAVE_SYSTEMD
SYSTEMD_UNIT = phosphor-ipmi-net@.service
SYSTEMD_SOCKET = phosphor-ipmi-net@.socket
systemdsystemunit_DATA = $(SYSTEMD_UNIT) $(SYSTEMD_SOCKET)
endif  # HAVE_SYSTEMD

SUBDIRS = test

