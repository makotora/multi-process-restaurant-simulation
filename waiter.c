#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "shared.h"

int main(int argc, char const *argv[])
{

	int i,err,max_period,max_money,tables_num,shared_id,pid;
	char * append_name = NULL;
	FILE * append_file;
	FILE * out;
	max_period = -1;
	max_money = -1;
	shared_id = -1;

	if (argc < 7)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./waiter -d period -m moneyamount -s shmid [-a appendfile]\n");
		return 1;
	}
	else
	{
		i = 1;
		while (i < argc)
		{
			if ( !strcmp(argv[i],"-d"))
			{
				max_period = atoi(argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-m"))
			{
				max_money = atoi(argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-s"))
			{
				shared_id = atoi(argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-a"))
			{
				append_name = malloc( (strlen(argv[i+1]) + 1) * sizeof(char) );
				strcpy(append_name,argv[i+1]);
				i += 2;
			}
			else
			{
				printf("Invalid argument given: %s\n",argv[i]);
				printf("Usage: ./waiter -d period -m moneyamount -s shmid [-a appendfile]\n");
				return 2;
			}

		}
	}

	if (max_period == -1)
	{
		printf("You didnt give the -d argument\n");
		printf("Usage: ./waiter -d period -m moneyamount -s shmid [-a appendfile]\n");
		return 3;
	}
	else if (max_money == -1)
	{
		printf("You didnt give the -m argument\n");	
		printf("Usage: ./waiter -d period -m moneyamount -s shmid [-a appendfile]\n");
		return 3;
	}
	else if (shared_id == -1)
	{
		printf("You didnt give the -s argument\n");
		printf("Usage: ./waiter -d period -m moneyamount -s shmid [-a appendfile]\n");
		return 3;
	}

	if (append_name != NULL)
	{
		append_file = fopen(append_name,"a");
		if (append_file == NULL)
		{
			fprintf(stderr, "Error opening appendfile '%s'\n",append_name);
			return 4;
		}
		out = append_file;
	}
	else
		out = stdout;


	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (int) shared_memory == -1)
	{
		perror("shmat");
	}

	shared_struct * shared_info = shared_memory;
	table * shared_tables = shared_memory + sizeof(shared_struct);
	tables_num = shared_info->tables_num;
	pid = getpid();

	sem_wait(&shared_info->append_file);
	fprintf(out, "\nWaiter %d arrived with max_period %d , max_money %d and shmdid %d\n",pid,max_period,max_money,shared_id);
	print_shared_struct(out, shared_info);
	print_shared_tables(out, shared_tables, shared_info->tables_num);
	fflush(out);
	sem_post(&shared_info->append_file);

	srand(time(NULL));

	while (1)
	{
		int did_something = 0;/*if it remains 0,this waiter didnt do anything*/
		int action_period,moneymoney;
		/*so he needs to 'wake' up another waiter to do something*/

		sem_wait(&shared_info->append_file);
		fprintf(out, "\nWaiter %d is waiting for someone to call him\n",pid);
		fflush(out);	
		sem_post(&shared_info->append_file);

		sem_wait(&shared_info->tables.waiter_busy);
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nWaiter %d was called by someone\n",pid);
		fflush(out);
		sem_post(&shared_info->append_file);


		/*If restaurant is closed.Go home*/
		if (shared_info->restaurant_open == 0)
			break;

		sem_wait(&shared_info->append_file);
		fprintf(out, "\nWaiter %d is looking at tables\n",pid);
		print_shared_tables(out, shared_tables, shared_info->tables_num);
		fflush(out);
		sem_post(&shared_info->append_file);
		action_period = ( rand() % max_period ) + 1;/*time action will take*/


		for (i=0;i<tables_num;i++)
		{

			if (shared_tables[i].group_id != -1 && shared_tables[i].waiter_id == -1)/*look if table has a group and no waiter*/
			{				
				/*make sure no other waiter is looking to "take" a table at the same time*/
				sem_wait(&shared_info->tables.waiter_table);
				if (shared_tables[i].waiter_id == -1)/*check again.maybe someone arrived at the table faster!*/
				{/*take this table if noone has already*/
					shared_tables[i].waiter_id = pid;
					sem_wait(&shared_info->append_file);
					fprintf(out, "\nWaiter %d took over table %d\n",pid,i);
					fflush(out);
					sem_post(&shared_info->append_file);
				}
				sem_post(&shared_info->tables.waiter_table);
			}

			if (shared_tables[i].waiter_id == pid)/*if this table is mine*/
			{
				if (shared_tables[i].group_activity == 0)/*if they want to order*/
				{
					did_something = 1;/*note that you did something*/

					sleep(action_period);/*take their order and bring their food*/
					sem_wait(&shared_info->append_file);
					fprintf(out, "\nWaiter %d took order from table %d and brought their food\n",pid,i);
					fflush(out);
					sem_post(&shared_info->append_file);
					sem_post(&(shared_tables[i].table_service));/*give them their food*/
					shared_tables[i].group_activity = 1;/*note that they are eating*/
					/*Note that only this waiter comes to this table*/
					/*So we noting that they are eating doesnt need a semaphore*/
					sem_wait(&shared_info->stats.stats_write);
					shared_info->stats.groups_serviced += 1;
					sem_post(&shared_info->stats.stats_write);
					break;
				}
				else if (shared_tables[i].group_activity == 2)/*if they want to pay*/
				{
					did_something = 1;/*note that you did something*/

					sleep(action_period);/*bring the check and get paid*/
					moneymoney = ( rand() % max_money ) + 1;
					sem_wait(&shared_info->append_file);
					fprintf(out, "\nWaiter %d brought the check and got paid %d$ from table %d\n",pid,moneymoney,i);
					fflush(out);
					sem_post(&shared_info->append_file);
					sem_post(&(shared_tables[i].table_service));/*say goodbye and thank you,dont forget about thank you*/

					sem_wait(&shared_info->stats.stats_write);
					shared_info->stats.income += moneymoney;/*Give cashier the money*/
					/*Dont steal any money!NO TIPS!*/
					shared_info->stats.people_at_tables -= shared_tables[i].group_size;
					shared_info->stats.groups_gone += 1;
					print_stats(out, shared_info);
					sem_post(&shared_info->stats.stats_write);

					/*CLEAN THE TABLE:reset table vars*/
					/*no need for semaphores,only this waiter is here*/
					shared_tables[i].group_id = -1;
					shared_tables[i].group_size = -1;
					shared_tables[i].group_activity = -1;
					shared_tables[i].waiter_id = -1;/*table is no longer mine*/
					/*ITS IMPORTANT TO DO ^THIS^ LAST*/

					break;
				}
			}
		}

		if (!did_something)/*If this waiter did nothing,call another waiter*/
		{
			sem_wait(&shared_info->append_file);
			fprintf(out, "\nWaiter %d is calling another waiter\n",pid);
			fflush(out);
			sem_post(&shared_info->append_file);
			sleep(action_period);/*find another waiter*/ 
			sem_post(&shared_info->tables.waiter_busy);/*call him*/
		}
	}


	sem_wait(&shared_info->append_file);
	fprintf(out, "\nWaiter %d leaving the restaurant\n",pid);
	fflush(out);
	sem_post(&shared_info->append_file);
	
	err = shmdt (( void *) shared_memory ) ; /* Detach segment */
	if ( err == -1)
	{
		perror ("shmdt") ;
	}

	return 0;

}