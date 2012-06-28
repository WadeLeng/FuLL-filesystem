CC      = gcc 
CFLAGS	=  -Wall -D_FILE_OFFSET_BITS=64 -g
TARGET1 = fufs
TARGET2 = init
OBJS1	= fufs.o op.o
OBJS2   = init.o
INC	=  -pthread -lfuse -lrt -ldl

all:init fufs
init:$(OBJS2)
	$(CC)   $(OBJS2) -o $(TARGET2)
fufs:$(OBJS1)
	$(CC)   $(CFLAGS) $(OBJS1) -o $(TARGET1) $(INC)
fufs.o:fufs.h 
op.o:op.h
init.o:fufs.h

.PHONY : all
clean :
	rm  -f $(TARGET1) $(TARGET2) $(OBJS1) $(OBJS2)









