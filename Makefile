OBJS = restaurant.o shared.o functions.o
EOBJS = doorman.o customer.o
EXECS = doorman customer
SOURCE	= restaurant.c shared.c doorman.c customer.c functions.c
HEADER  = shared.h functions.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -pthread -Wall $(DEBUG)

restaurant: $(OBJS) $(EOBJS) $(EXECS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT)

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

doorman: shared.o doorman.o
	$(CC) $(LFLAGS) shared.o doorman.o -o doorman

customer: shared.o customer.o
	$(CC) $(LFLAGS) shared.o customer.o -o customer

shared.o: shared.c
	$(CC) $(CFLAGS) shared.c

doorman.o: doorman.c
	$(CC) $(CFLAGS) doorman.c

customer.o: customer.c
	$(CC) $(CFLAGS) customer.c

functions.o: functions.c
	$(CC) $(CFLAGS) functions.c

clean:
	rm -f $(OBJS) $(EOBJS) $(EXECS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

