OBJS = restaurant.o shared.o functions.o
EXECS = doorman
SOURCE	= restaurant.c shared.c doorman.c functions.c
HEADER  = shared.h functions.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -pthread -Wall $(DEBUG)

restaurant: $(OBJS) $(EXECS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT)

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

doorman: shared.o doorman.o
	$(CC) $(LFLAGS) shared.o doorman.o -o doorman

shared.o: shared.c
	$(CC) $(CFLAGS) shared.c

doorman.o: doorman.c
	$(CC) $(CFLAGS) doorman.c

functions.o: functions.c
	$(CC) $(CFLAGS) functions.c

clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

