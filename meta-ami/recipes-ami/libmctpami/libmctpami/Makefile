#------- Comment/uncomment the following line to enable/disable debugging---------
DEBUG = y

#---------------------- Change according to your files ----------------------------
SRC 	= mctp_dbus.c # Source File
LIBRARY_NAME = libmctpami# Output file

# Add the Corresponding library

LIBS+= -lmctpw
LIBS+= -ldl -lsystemd
LIBS+= -Wl,-export-dynamic

CFLAGS+= -fPIC

PKG_MAJOR = 0
PKG_MINOR = 0
PKG_AUX   = 0

LIBS += -Wl,-rpath=/usr/local/lib -Wl,-rpath=/usr/lib -Wl,-rpath=/lib

ifeq ($(DEBUG),y)
  DEBFLAGS = -g -DDEBUG # "-O" is removed because it is not allowing gdb to show local variables
else
  ifeq ($(GNU_DEBUG),y)
    DEBFLAGS = -g
  else  
    DEBFLAGS = -O2
  endif
endif

CFLAGS += $(DEBFLAGS) 

#include ${TOOLDIR}/rules/Rules.make.apps
#this stuff is mostly used within makefiles inside the libs directory
# this simplifies concepts such as linker name,shared object name etc
TARGET = $(LIBRARY_NAME).a

# Name of the library shared object
LIB_COMMON_LIBNAME = $(LIBRARY_NAME).so

# These are unix (linux) standards for naming shared libraries.
# Stick to these conventions for naming.
# Linux HOWTO http://www.tldp.org/HOWTO/Program-Library-HOWTO/introduction.html
# for details
LIB_SONAME = $(LIB_COMMON_LIBNAME).${PKG_MAJOR}
LIB_REALNAME = $(LIB_COMMON_LIBNAME).${PKG_MAJOR}.${PKG_MINOR}.${PKG_AUX}
LIB_ARCHIVE_NAME = $(LIBRARY_NAME).a

.c.o:
#	@echo "$(CC): creating $@ ..."
	@echo "MDS Version:$(CONFIG_SPX_FEATURE_MDS_VERSION)"
	@echo "  CC      $@ "
	@$(CC) -c $(CFLAGS) $(CCFLAGS) $< -o $@ 

all:   .depend $(LIB_REALNAME)	# If your module exports symbols

$(LIB_ARCHIVE_NAME): $(SRC:.c=.o) $(SRCCPP:.cpp=.o)
	@echo "  AR      $@"
	$(AR) $(ARFLAGS) $@ $^ 2>/dev/null

$(LIB_REALNAME):$(LIB_ARCHIVE_NAME)
	@echo "  LIB     $@ "
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(LIB_SONAME) -o $(LIB_REALNAME) \
	-Wl,--whole-archive $(LIB_ARCHIVE_NAME) -Wl,--no-whole-archive -lc $(LIBS)
	
depend .depend dep:
	@echo "  DEP     $(LIB_REALNAME)"
	@echo "  CC  	$(CC)"
	@echo "  CFLAGS  $(CFLAGS)"
	@echo "  SRC     $(SRC)"
	$(CC) $(CFLAGS) -M  $(SRC) > $@

clean:
	@rm -f *.o *~ core .depend $(TARGET) *.lcd
	@rm -f $(LIB_COMMON_LIBNAME) $(LIB_REALNAME)

ifeq (.depend,$(wildcard .depend))
include .depend
endif

#----------------------------------------------------------------------------------------
