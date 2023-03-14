CC = gcc
CFLAGS = -fPIC -Wall
LDFLAGS = 

.PHONY: build
build: libscheduler.so copy

libscheduler.so: so_scheduler.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

so_scheduler.o: so_scheduler.c queue.c
	$(CC) $(CFLAGS) -o $@ -c $<

copy:
	cp libscheduler.so ../checker-lin

.PHONY: clean
clean:
	-rm -f so_scheduler.o so_scheduler.so