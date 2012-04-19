# Makefile for httpsqs
CC=g++
CFLAGS=-L/usr/local/libevent-2.0.10-stable/lib/ -levent -I./leveldb/include -L./leveldb -lleveldb -I/usr/local/libevent-2.0.10-stable/include/ -lz  -lrt -lpthread -lm -lc -g 

server: server.c handler.c utils.c database.c
	$(CC) -o server server.c handler.c utils.c database.c $(CFLAGS)
	@echo "server build complete."
	@echo ""

clean: server
	rm -f server

install: server
	install $(INSTALL_FLAGS) -m 4755 -o root server $(DESTDIR)/usr/bin
