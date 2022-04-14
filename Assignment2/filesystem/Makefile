SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
HDRS := $(wildcard *.h)

CFLAGS := -g `pkg-config fuse --cflags`
LDLIBS := `pkg-config fuse --libs`

fs: $(OBJS)
	gcc $(CLFAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HDRS)
	gcc $(CFLAGS) -fno-stack-protector -c -o $@ $<

clean: unmount
	rm -f fs *.o test.log data.fs
	rmdir mnt || true

mount: fs
	mkdir -p mnt || true
	./fs -s -f mnt data.fs

unmount:
	fusermount -u mnt || true

test: fs
	perl test.pl

gdb: fs
	mkdir -p mnt || true
	gdb --args ./fs -s -f mnt data.fs

.PHONY: clean mount unmount gdb