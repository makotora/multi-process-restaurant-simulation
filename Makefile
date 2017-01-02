OBJS = queue.o restaurant.o tables.o
SOURCE	= restaurant.c tables.c queue.c
HEADER  = tables.h queue.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

restaurant: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT)

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

tables.o: tables.c
	$(CC) $(CFLAGS) tables.c

queue.o: queue.c
	$(CC) $(CFLAGS) queue.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

