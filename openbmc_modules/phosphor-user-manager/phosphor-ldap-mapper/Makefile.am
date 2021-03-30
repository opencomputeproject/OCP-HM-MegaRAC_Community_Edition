bin_PROGRAMS = phosphor-ldap-mapper

noinst_HEADERS = ldap_mapper_mgr.hpp \
                 ldap_mapper_entry.hpp \
                 ldap_mapper_serialize.hpp

phosphor_ldap_mapper_SOURCES = \
                main.cpp \
                ldap_mapper_mgr.cpp \
                ldap_mapper_entry.cpp \
                ldap_mapper_serialize.cpp

phosphor_ldap_mapper_LDFLAGS = $(SDBUSPLUS_LIBS) \
                               $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                               $(PHOSPHOR_LOGGING_LIBS) \
                               -lstdc++fs

phosphor_ldap_mapper_CXXFLAGS = $(SYSTEMD_CFLAGS) \
                                $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                                $(PHOSPHOR_LOGGING_CFLAGS) \
                                -flto
