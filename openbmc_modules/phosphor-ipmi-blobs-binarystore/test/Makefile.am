AM_CPPFLAGS = -I$(top_srcdir)/ \
	-I$(builddir)
	$(GTEST_CFLAGS) \
	$(GMOCK_CFLAGS)
AM_CXXFLAGS = \
	$(GTEST_MAIN_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)
AM_LDFLAGS = \
	$(GMOCK_LIBS) \
	$(GTEST_MAIN_LIBS) \
	$(OESDK_TESTCASE_FLAGS) \
	$(PHOSPHOR_LOGGING_LIBS)

# Run all 'check' test programs
check_PROGRAMS = \
	parse_config_unittest \
	sys_file_unittest \
	handler_commit_unittest \
	handler_stat_unittest \
	handler_open_unittest \
	handler_readwrite_unittest \
	handler_unittest
TESTS = $(check_PROGRAMS)

parse_config_unittest_SOURCES = parse_config_unittest.cpp

sys_file_unittest_SOURCES = sys_file_unittest.cpp
sys_file_unittest_LDADD = $(top_builddir)/sys_file_impl.o

handler_unittest_SOURCES = handler_unittest.cpp
handler_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) \
	$(top_builddir)/handler.o \
	$(top_builddir)/binarystore.o \
	$(top_builddir)/sys_file_impl.o \
	$(top_builddir)/libbinarystore_la-binaryblob.pb.o \
	-lprotobuf
handler_unittest_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)

handler_commit_unittest_SOURCES = handler_commit_unittest.cpp
handler_commit_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) \
	$(top_builddir)/handler.o \
	$(top_builddir)/binarystore.o \
	$(top_builddir)/sys_file_impl.o \
	$(top_builddir)/libbinarystore_la-binaryblob.pb.o \
	-lprotobuf
handler_commit_unittest_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)

handler_stat_unittest_SOURCES = handler_stat_unittest.cpp
handler_stat_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) \
	$(top_builddir)/handler.o \
	$(top_builddir)/binarystore.o \
	$(top_builddir)/sys_file_impl.o \
	$(top_builddir)/libbinarystore_la-binaryblob.pb.o \
	-lprotobuf
handler_stat_unittest_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)

handler_open_unittest_SOURCES = handler_open_unittest.cpp
handler_open_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) \
	$(top_builddir)/handler.o \
	$(top_builddir)/binarystore.o \
	$(top_builddir)/sys_file_impl.o \
	$(top_builddir)/libbinarystore_la-binaryblob.pb.o \
	-lprotobuf
handler_open_unittest_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)

handler_readwrite_unittest_SOURCES = handler_readwrite_unittest.cpp
handler_readwrite_unittest_LDADD = $(PHOSPHOR_LOGGING_LIBS) \
	$(top_builddir)/handler.o \
	$(top_builddir)/binarystore.o \
	$(top_builddir)/sys_file_impl.o \
	$(top_builddir)/libbinarystore_la-binaryblob.pb.o \
	-lprotobuf
handler_readwrite_unittest_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)
