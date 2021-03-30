noinst_HEADERS += \
        offload-extensions/pldm/pldm_interface.hpp

phosphor_dump_manager_LDADD += \
	$(LIBPLDM_LIBS)

phosphor_dump_manager_CXXFLAGS += \
        $(LIBPLDM_CFLAGS)

phosphor_dump_manager_SOURCES += \
       offload-extensions/pldm/pldm_interface.cpp

