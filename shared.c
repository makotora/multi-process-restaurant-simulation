#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "shared.h"

void print_shared_struct(FILE * out, shared_struct * ss)
{
	int sem;

	fprintf(out, "tables_num %d\n", ss->tables_num);
	fprintf(out, "restaurant_open %d\n", ss->restaurant_open);
	sem_getvalue(&ss->append_file,&sem);
	fprintf(out, "append_file %d\n", sem);
	sem_getvalue(&ss->doorman.doorman_busy,&sem);

	fprintf(out, "doorman_busy %d\n", sem);
	sem_getvalue(&ss->doorman.doorman_answer,&sem);
	fprintf(out, "doorman_answer %d\n", sem);
	sem_getvalue(&ss->doorman.doorman_bar_answer,&sem);
	fprintf(out, "doorman_bar_answer %d\n", sem);
	fprintf(out, "answer %d\n", ss->doorman.answer);
	fprintf(out, "bar_answer %d\n", ss->doorman.bar_answer);

	sem_getvalue(&ss->door.door_queue,&sem);
	fprintf(out, "door_queue %d\n", sem);
	sem_getvalue(&ss->door.door_group_size,&sem);
	fprintf(out, "door_group_size semaphore %d\n", sem);
	fprintf(out, "door_group_size %d\n", ss->door.group_size);

	sem_getvalue(&ss->bar.bar_queue,&sem);
	fprintf(out, "bar_queue %d\n", sem);
	sem_getvalue(&ss->bar.bar_group_size,&sem);
	fprintf(out, "bar_group_size semaphore %d\n", sem);
	fprintf(out, "bar_groups_num %d\n", ss->bar.groups_num);
	fprintf(out, "bar_group_size %d\n", ss->bar.group_size);
	fprintf(out, "bar_capacity %d\n", ss->bar.capacity);

	sem_getvalue(&ss->tables.waiter_busy,&sem);
	fprintf(out, "waiter_busy %d\n", sem);
	sem_getvalue(&ss->tables.waiter_table,&sem);
	fprintf(out, "waiter_queue %d\n", sem);
	sem_getvalue(&ss->tables.table_change,&sem);
	fprintf(out, "table_change %d\n", sem);
	fprintf(out, "free_tables %d\n", ss->tables.free_tables);

	sem_getvalue(&ss->stats.stats_write,&sem);
	fprintf(out, "stats_write %d\n", sem);
	fprintf(out, "people_at_tables %d\n",ss->stats.people_at_tables);
	fprintf(out, "people_at_bar %d\n",ss->stats.people_at_bar);
	fprintf(out, "groups_serviced %d\n",ss->stats.groups_serviced);
	fprintf(out, "groups_gone %d\n",ss->stats.groups_gone);
	fprintf(out, "income %d\n",ss->stats.income);
	fprintf(out, "groups_bored_waiting %d\n",ss->stats.groups_bored_waiting);

}


void print_stats(FILE * out,shared_struct * ss)
{
	fprintf(out, "\n--Printing restaurant statistics--\n");
	fprintf(out, "People at tables: %d\n",ss->stats.people_at_tables);
	fprintf(out, "Groups serviced: %d\n",ss->stats.groups_serviced);
	fprintf(out, "Groups gone after paying: %d\n",ss->stats.groups_gone);
	
	fprintf(out, "People at bar: %d\n",ss->stats.people_at_bar);
	fprintf(out, "Groups sent at the bar: %d\n",ss->stats.bar_groups);
	fprintf(out, "Groups bored waiting at the bar: %d\n",ss->stats.groups_bored_waiting);
	fprintf(out, "Groups gone because restaurant was full: %d\n",ss->stats.door_groups_gone);

	fprintf(out, "Total income: %d\n",ss->stats.income);
}


void print_shared_tables(FILE * out, table * tables, int tables_num)
{
	int i,sem;
	for (i=0;i<tables_num;i++)
	{
		sem_getvalue(&(tables[i].table_service),&sem);
		fprintf(out, "Table %d:capacity %d, group_id %d, group_size %d, waiter_id %d, group_activity %d income %d table_service %d\n"
			,i,tables[i].capacity,tables[i].group_id,tables[i].group_size,
			tables[i].waiter_id,tables[i].group_activity,tables[i].income,sem);
	}
}

void print_shared_tables_incomes(FILE * out, table * tables, int tables_num)
{
	int i;
	for (i=0;i<tables_num;i++)
	{
		fprintf(out, "[Table %d: Income %d] ",i,tables[i].income);
		if ( (i+1) % 6 == 0 || i == (tables_num -1) )
			putchar('\n');
	}
}