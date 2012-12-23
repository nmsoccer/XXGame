CC = gcc
FLAGS = -g -Wall
LIBOBJS = ../tool.o ../mempoll.o ../XXBUS/xx_bus.o
INCLUDE = ..

log_server:log_server.o
	$(CC) log_server.o $(LIBOBJS) -o $@
log_server.o:log_server.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
clean:
	rm *.o
	rm log_server	 	