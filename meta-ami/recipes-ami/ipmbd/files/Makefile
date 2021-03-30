all: ipmbd

ipmbd: ipmbd.o
	$(CC) $(CFLAGS) -pthread -lrt -std=gnu99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o ipmbd
