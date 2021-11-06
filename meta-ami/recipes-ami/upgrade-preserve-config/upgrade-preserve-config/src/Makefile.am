bin_PROGRAMS = upgrade-preserve-config

noinst_HEADERS = \
		 conf_manager.hpp \
		 preserve.hpp \
		 preserve_serialize.hpp

upgrade_preserve_config_SOURCES = \
				  main.cpp \
				  conf_manager.cpp \
				  preserve.cpp \
				  preserve_serialize.cpp \
				  xyz/openbmc_project/Software/Preserve/server.cpp

CLEANFILES = \
	     xyz/openbmc_project/Software/Preserve/server.cpp \
	     xyz/openbmc_project/Software/Preserve/server.hpp

BUILT_SOURCES = \
	     xyz/openbmc_project/Software/Preserve/server.cpp \
	     xyz/openbmc_project/Software/Preserve/server.hpp

upgrade_preserve_config_LDFLAGS = \
				  $(SYSTEMD_LIBS) \
				  $(SDBUSPLUS_LIBS) \
				  $(PHOSPHOR_LOGGING_LIBS) \
				  -lstdc++fs

upgrade_preserve_config_CXXFLAGS = \
				   $(SYSTEMD_CFLAGS) \
				   $(SDBUSPLUS_CFLAGS) \
				   $(PHOSPHOR_LOGGING_CFLAGS)

xyz/openbmc_project/Software/Preserve/server.cpp: xyz/openbmc_project/Software/Preserve.interface.yaml xyz/openbmc_project/Software/Preserve/server.hpp
		@mkdir -p `dirname $@`
			$(SDBUSPLUSPLUS) -r $(srcdir) interface server-cpp xyz.openbmc_project.Software.Preserve > $@

xyz/openbmc_project/Software/Preserve/server.hpp: xyz/openbmc_project/Software/Preserve.interface.yaml
		@mkdir -p `dirname $@`
			$(SDBUSPLUSPLUS) -r $(srcdir) interface server-header xyz.openbmc_project.Software.Preserve > $@
				sed -i '5i #include \"xyz\/openbmc_project\/Software\/Preserve\/server.hpp\"' $@
