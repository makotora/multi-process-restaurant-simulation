#include <stdio.h>
#include <stdlib.h>

#include "tables.h"

int tables_create(table ** table_ptr_ptr,int table_num,int * table_capacities)
{
	table * tables = malloc(table_num*sizeof(table));
	if (tables == NULL)
		return 10;

	int i;
	for (i=0;i<table_num;i++)
	{
		tables[i].group_id = -1;
		tables[i].group_activity = -1;
		tables[i].waiter_id = -1;
		tables[i].capacity = table_capacities[i];
	}

	*table_ptr_ptr = tables;

	return 0;
}


int tables_free(table ** table_ptr_ptr)
{
	free(*table_ptr_ptr);

	return 0;
}