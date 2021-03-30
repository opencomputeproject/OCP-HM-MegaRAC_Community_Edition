bin_PROGRAMS = phosphor-ldap-conf

noinst_HEADERS = ldap_config.hpp \
            ldap_config_mgr.hpp \
            ldap_mapper_entry.hpp \
            ldap_mapper_serialize.hpp \
            utils.hpp

phosphor_ldap_conf_SOURCES = \
                main.cpp \
                utils.cpp \
                ldap_config.cpp \
                ldap_config_mgr.cpp \
                ldap_mapper_entry.cpp \
                ldap_mapper_serialize.cpp

phosphor_ldap_conf_LDFLAGS = $(SDBUSPLUS_LIBS) \
                             $(PHOSPHOR_DBUS_INTERFACES_LIBS) \
                             $(PHOSPHOR_LOGGING_LIBS) \
                             -lstdc++fs \
                             -lldap

phosphor_ldap_conf_CXXFLAGS = $(SYSTEMD_CFLAGS) \
                              $(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
                              $(PHOSPHOR_LOGGING_CFLAGS) \
                              -flto
