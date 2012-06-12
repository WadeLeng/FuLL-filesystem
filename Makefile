CC = g++ -O2
CFLAGS=-L/usr/local/libevent-2.0.10-stable/lib/ -levent -I./leveldb/include -L./leveldb -lleveldb -I/usr/local/libevent-2.0.10-stable/include/ -lz  -lrt -lpthread -lm -lc -g `pkg-config fuse --cflags --libs`
SRC = server.c utils.c fuse.c handler.c database.c
HEADERS = server.h utils.h fuse.h handler.h database.h
OBJ = full_server

$(OBJ): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(OBJ) $(SRC)
	@echo "FuLL_server build complete."

all:
	make $(OBJ)

clean: 
	rm -f $(OBJ)
