CC = g++ -O2
CFLAGS=-L/usr/local/libevent-2.0.12-stable/lib/ -levent -I./leveldb/include -L./leveldb -lleveldb -I/usr/local/libevent-2.0.12-stable/include/ -lz  -lrt -lpthread -lm -lc -g `pkg-config fuse --cflags --libs`
SRC = server.c utils.c  handler.c database.c
HEADERS = database.h utils.h  handler.h 
OBJ = full_server

$(OBJ): $(SRC) $(HEADERS)
	$(CC) $(SRC) $(CFLAGS) -o $(OBJ) 
	@echo "FuLL_server build complete."

all:
	make $(OBJ)

clean: 
	rm -f $(OBJ)
