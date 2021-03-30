
obj = pwmtach.o pwmtachtool.o EINTR_wrappers.o
target = pwmtachtool

all: $(obj)
	${CC} $(obj) ${LDFLAGS} -o $(target)

%.o:%.c
	${CC} -c $^ -o $@

.PHONY: clean
clean:
	rm -rf $(obj) $(target)
