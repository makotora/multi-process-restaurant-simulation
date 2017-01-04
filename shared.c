#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "shared.h"

void print_shared_struct(shared_struct * ss)
{
	int sem;

	printf("tables_num %d\n", ss->tables_num);
	sem_getvalue(&ss->doorman.doorman_busy,&sem);
	printf("doorman_busy %d\n", sem);
	printf("doorman_answer %d\n", ss->doorman.answer);
	sem_getvalue(&ss->door.door_queue,&sem);
	printf("door_queue %d\n", sem);
	printf("group_size %d\n", ss->door.group_size);
	sem_getvalue(&ss->bar.bar_queue,&sem);
	printf("bar_queue %d\n", sem);
	printf("group_size %d\n", ss->bar.group_size);
	sem_getvalue(&ss->tables.waiter_busy,&sem);
	printf("waiter_busy %d\n", sem);
	sem_getvalue(&ss->tables.waiter_queue,&sem);
	printf("waiter_queue %d\n", sem);

}


void print_shared_tables(table * tables, int tables_num)
{
	int i;
	for (i=0;i<tables_num;i++)
	{
		printf("Table %d:capacity %d, group_id %d, waiter_id %d, group_activity %d\n"
			,i+1,tables[i].capacity,tables[i].group_id,tables[i].waiter_id,tables[i].group_activity);
	}
}