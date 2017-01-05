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

	int i,err,max_period,service_time,max_money,tables_num,shared_id,pid;
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
		out = stderr;


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


	sem_wait(&shared_info->append_file);
	fprintf(out, "\nDoorman leaving the restaurant\n");
	fflush(out);
	sem_post(&shared_info->append_file);
	
	err = shmdt (( void *) shared_memory ) ; /* Detach segment */
	if ( err == -1)
	{
		perror ("shmdt") ;
	}

	return 0;

}