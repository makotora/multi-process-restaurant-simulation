#include <stdio.h>
#include <stdlib.h>

#include "tables.h"
#include "queue.h"

#define TABLES 5

int main()
{
	int * table_capacities = malloc(TABLES*sizeof(int));
	int i;
	int capacity = 2;
	for (i=0;i<TABLES;i++)
	{
		table_capacities[i] = capacity;
		capacity *= 2;
		if (capacity > 8)
			capacity = 2;
	}


	printf("--TESTING TABLES--\n");
	table * tables;
	tables_create(&tables,TABLES,table_capacities);
	for (i=0;i<TABLES;i++)
	{
		printf("TABLE %d: capacity is %d\n",i+1,tables[i].capacity);
	}

	tables_free(&tables);
	printf("--DONE TESTING TABLES--\n\n");

	printf("--TESTING QUEUE--\n");
	queue * door;
	queue_create(&door);
	queue_push(door,1);
	queue_push(door,2);
	queue_push(door,3);
	queue_push(door,4);
	queue_push(door,5);
	printf("Popped: %d\n",queue_pop(door));
	printf("Popped: %d\n",queue_pop(door));
	printf("Popped: %d\n",queue_pop(door));
	printf("Popped: %d\n",queue_pop(door));
	printf("Popped: %d\n",queue_pop(door));
	printf("Popped: %d\n",queue_pop(door));
	
	queue_free(&door);
	printf("--DONE TESTING QUEUE--\n\n");

	free(table_capacities);

	return 0;
}