nobase_include_HEADERS =
pkgconfig_DATA = ipmiblob.pc
lib_LTLIBRARIES = libipmiblob.la
libipmiblob_la_SOURCES =
libipmiblob_la_LIBADD = $(COMMON_LIBS)

nobase_include_HEADERS += ipmiblob/crc.hpp
libipmiblob_la_SOURCES += ipmiblob/crc.cpp

nobase_include_HEADERS += ipmiblob/blob_errors.hpp
nobase_include_HEADERS += ipmiblob/blob_interface.hpp
nobase_include_HEADERS += ipmiblob/blob_handler.hpp
libipmiblob_la_SOURCES += ipmiblob/blob_handler.cpp

nobase_include_HEADERS += ipmiblob/ipmi_interface.hpp
nobase_include_HEADERS += ipmiblob/ipmi_handler.hpp
libipmiblob_la_SOURCES += ipmiblob/ipmi_handler.cpp

nobase_include_HEADERS += ipmiblob/internal/sys.hpp
libipmiblob_la_SOURCES += ipmiblob/internal/sys.cpp

nobase_include_HEADERS += ipmiblob/test/blob_interface_mock.hpp
nobase_include_HEADERS += ipmiblob/test/crc_mock.hpp
nobase_include_HEADERS += ipmiblob/test/ipmi_interface_mock.hpp
