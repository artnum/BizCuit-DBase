CC=gcc
CFLAGS=
LDFLAGS=
EXEC=db5-dump db5-head

all: $(EXEC)

db5-head: db5-head.o libdbase5.so
	$(CC) db5-head.o libdbase5.so -o db5-head

db5-dump: db5-dump.o libdbase5.so
	$(CC) db5-dump.o libdbase5.so -o db5-dump

db5-dump.o: db5-dump.c
	$(CC) db5-dump.c -c -o db5-dump.o

db5-head.o: db5-head.c
	$(CC) db5-head.c -c -o db5-head.o

libdbase5.so: dbase.c memo.c cache.c
	$(CC) dbase.c memo.c cache.c -fPIC -shared -o libdbase5.so

install: libdbase5.so test head
	cp libdbase5.so /usr/local/lib/
	cp db5-head /usr/local/bin/
	cp db5-dump /usr/local/bin/
	ldconfig

.PHONY: clean

clean:
	rm -rf *.o
	rm -rf libdbase5.so
