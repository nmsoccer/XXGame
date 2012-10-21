CC = gcc
FLAGS = -g -Wall
LIBOBJS = ../tool.o ../mempoll.o ../XXBUS/xx_bus.o
INCLUDE = ..

logic_server:logic_server.o
	$(CC) logic_server.o $(LIBOBJS) -o $@
logic_server.o:logic_server.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
clean:
	rm *.o
	rm logic_server	 	
