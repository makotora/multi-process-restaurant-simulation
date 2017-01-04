OBJS = restaurant.o
SOURCE	= restaurant.c
HEADER  = shared.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

restaurant: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT)

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

