bin_PROGRAMS = phosphor-user-manager

noinst_HEADERS = user_mgr.hpp users.hpp

phosphor_user_manager_SOURCES = \
                mainapp.cpp \
                user_mgr.cpp \
                users.cpp

phosphor_user_manager_LDFLAGS = $(SDBUSPLUS_LIBS) \
                                $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                                $(PHOSPHOR_LOGGING_LIBS) \
                                $(BOOST_CPPFLAGS) \
                                -lcrypt \
                                -lstdc++fs

phosphor_user_manager_CXXFLAGS = $(SYSTEMD_CFLAGS) \
                                 $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                                 $(PHOSPHOR_LOGGING_CFLAGS) \
                                 $(BOOST_CPPFLAGS) \
                                 -DBOOST_ALL_NO_LIB \
                                 -DBOOST_SYSTEM_NO_DEPRECATED \
                                 -DBOOST_ERROR_CODE_HEADER_ONLY

SUBDIRS = . phosphor-ldap-mapper phosphor-ldap-config test

