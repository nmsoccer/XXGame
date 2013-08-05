CC = gcc
FLAGS = -g -Wall
INCLUDE = ..
LIBOBJS = ../tool.o -ldl

all:create_bus delete_bus
	echo "set up bus...";
	
create_bus:create_bus.o
	$(CC) create_bus.o $(LIBOBJS) -o $@
create_bus.o:create_bus.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
delete_bus:delete_bus.o
	$(CC) delete_bus.o $(LIBOBJS) -o $@
delete_bus.o:delete_bus.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
clean:
	rm *.o
	rm create_bus
	rm delete_bus		 			