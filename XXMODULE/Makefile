CC=gcc
FLAGS=-g -Wall
LINKFLAG=-shared -lc 
LIBOBJS = ../tool.o ../mempoll.o ../XXBUS/xx_bus.o
INCLUDE = ..

all:cs_module1.so cs_module2.so cs_module0.so
	echo "Building Modules..."
	
cs_module1.so:cs_module1.o
	$(CC)  $(LINKFLAG) $(LIBOBJS) cs_module1.o -o $@
cs_module1.o:cs_module1.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
cs_module2.so:cs_module2.o
	$(CC)  $(LINKFLAG) $(LIBOBJS) cs_module2.o -o $@
cs_module2.o:cs_module2.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
cs_module0.so:cs_module0.o
	$(CC)  $(LINKFLAG) $(LIBOBJS) cs_module0.o -o $@
cs_module0.o:cs_module0.c
	$(CC) -I$(INCLUDE) $(FLAGS) -c $<
	
clean:
	rm *.o
	rm *.so