OBJS = restaurant.o shared.o
EOBJS = doorman.o customer.o waiter.o
EXECS = doorman customer waiter
SOURCE	= restaurant.c shared.c doorman.c customer.c waiter.c
HEADER  = shared.h
OUT  	= restaurant

CC=gcc
DEBUG= -g
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

restaurant: $(OBJS) $(EOBJS) $(EXECS)
	$(CC) $(LFLAGS) $(OBJS) -o $(OUT) -lpthread

restaurant.o: restaurant.c
	$(CC) $(CFLAGS) restaurant.c

doorman: shared.o doorman.o
	$(CC) $(LFLAGS) shared.o doorman.o -o doorman -lpthread

waiter: shared.o waiter.o
	$(CC) $(LFLAGS) shared.o waiter.o -o waiter -lpthread

customer: shared.o customer.o
	$(CC) $(LFLAGS) shared.o customer.o -o customer -lpthread

shared.o: shared.c
	$(CC) $(CFLAGS) shared.c

doorman.o: doorman.c
	$(CC) $(CFLAGS) doorman.c

waiter.o: waiter.c
	$(CC) $(CFLAGS) waiter.c

customer.o: customer.c
	$(CC) $(CFLAGS) customer.c

clean:
	rm -f $(OBJS) $(EOBJS) $(EXECS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)

