TARGET	= libsnoop.so
SRC	=  snoopifc.c
OBJECTS = $(SRC:.c=.o)

#CFLAGS += -I${SPXINC}/snoop
#CFLAGS += -I${SPXINC}/global
#CFLAGS += -I${SPXINC}/dbgout
LDFLAGS     += -shared

all: $(TARGET)

#$(TARGET): $(OBJECTS)
#	$(CC) -o $(CFLAGS) $(TARGET) $(OBJECTS) -shared
$(TARGET): $(OBJECTS)
	$(CC) ${LDFLAGS} -o $@ $^
clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers
#------------------------------------------------------------------------------
