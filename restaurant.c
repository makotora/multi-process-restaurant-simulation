#include <stdio.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/types.h>

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
		capacity += 2;
		if (capacity > 8)
			capacity = 2;
	}


	printf("--INITIALISING TABLES--\n");
	table * tables;
	tables_create(&tables,TABLES,table_capacities);
	for (i=0;i<TABLES;i++)
	{
		printf("TABLE %d: capacity is %d\n",i+1,tables[i].capacity);
	}
	printf("--DONE INITIALISING TABLES--\n\n");


	/*Create shared memory*/
	/*Initialise shared memory (semaphores too)*/
	/*Copy tables array in shared memory (after sharedStruct)*/



	tables_free(&tables);
	free(table_capacities);

	return 0;
}