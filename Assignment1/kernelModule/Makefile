obj-m += km1.o
obj-m += km2.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc setlevel_in.c -o setlevel_in
	gcc setlevel_rm.c -o setlevel_rm

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm setlevel_in
	rm setlevel_rm