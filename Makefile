CC = gcc
MAKE = make
SUBDIR = XXCONNECT XXBUS
OBJS = tool.o XXBUS/xx_bus.o
FLAGS = -g -Wall
INCLUDE = ..

all:$(OBJS)
	for dir in $(SUBDIR);\
	do \
	echo "Building $$dir";\
	$(MAKE) -C $$dir;\
	done	
		
tool.o:tool.c
	$(CC) $(FLAGS) -c $<	
XXBUS/xx_bus.o:XXBUS/xx_bus.c
	$(CC) $(FLAGS) -c $<	
clean:
	rm *.o
	for dir in $(SUBDIR);\
	do \
	echo "Clean $$dir...";\
	$(MAKE) clean -C $$dir;\
	done
#	cd XXCONNECT && $(MAKE) clean	