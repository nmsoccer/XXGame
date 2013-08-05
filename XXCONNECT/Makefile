CC = gcc
FLAGS = -g -Wall -lpthread
INCLUDE =  ..
LIBOBJS = ../tool.o ../mempoll.o ../XXBUS/xx_bus.o -ldl

connect_server:connect_server.o $(LIBOBJS)
	$(CC) connect_server.o $(LIBOBJS) -o $@
	
connect_server.o:connect_server.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
#../tool.o:../tool.c
#	$(CC) -c $<

clean:
	rm *.o
	rm connect_server	