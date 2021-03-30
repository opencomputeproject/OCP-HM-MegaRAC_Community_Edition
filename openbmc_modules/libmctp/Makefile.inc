LIBMCTP_DIR ?= libmctp/
LIBMCTP_OBJS = core.o alloc.o log.o
LIBMCTP_BINDINGS ?= serial astlpc

LIBMCTP_OBJS += $(LIBMCTP_BINDINGS:%=%.o)

LIBMCTP = $(LIBMCTP_DIR)libmctp.a

$(LIBMCTP): $(LIBMCTP_OBJS:%=$(LIBMCTP_DIR)%)
