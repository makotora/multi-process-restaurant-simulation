OBJS = restaurant.o shared.o
SOURCE	= restaurant.c shared.c
HEADER  = shared.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -pthread -Wall $(DEBUG)

restaurant: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT)

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

shared.o: shared.c
	$(CC) $(CFLAGS) shared.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

