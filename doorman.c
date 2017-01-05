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
	int i,err,max_period,sleep_time,shared_id;
	char * append_name = NULL;
	FILE * append_file = NULL;
	FILE * out;
	max_period = -1;
	shared_id = -1;

	if (argc < 5)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./doorman -d time -s shmid [-a appendfile]\n");
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
				printf("Usage: ./doorman -d time -s shmid [-a appendfile]\n");
				return 2;
			}

		}
	}

	if (max_period == -1)
	{
		printf("You didnt give the -d argument\n");
		printf("Usage: ./doorman -d time -s shmid [-a appendfile]\n");
		return 3;
	}
	else if (shared_id == -1)
	{
		printf("You didnt give the -s argument\n");
		printf("Usage: ./doorman -d time -s shmid [-a appendfile]\n");
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
	{
		out = stderr;
	}

	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (int) shared_memory == -1)
	{
		perror("shmat");
	}

	shared_struct * shared_info = shared_memory;
	table * shared_tables = shared_memory + sizeof(shared_struct);
	int tables_num = shared_info->tables_num;
	
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nDoorman arrived with max_time %d and shmdid %d\n",max_period,shared_id);
	// print_shared_struct(out, shared_info);
	// print_shared_tables(out, shared_tables, shared_info->tables_num);
	fflush(out);
	sem_post(&shared_info->append_file);


	while (1)
	{/*when doorman has nothing to do its closing time and we break*/
		/*wait for someone to call*/
		int group_size,best_table,best_table_size,table_size,bar_space;
		
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nDoorman is waiting for someone to call him\n");
		fflush(out);	
		sem_post(&shared_info->append_file);

		sem_wait(&shared_info->doorman.doorman_busy);

		/*First look at bar if a table was freed*/
		/*TO BE CONTINUED*/
		group_size = shared_info->door.group_size;
		
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nDoorman called by someone\n");
		//print_shared_struct(out, shared_info);
		print_shared_tables(out, shared_tables, shared_info->tables_num);
		fflush(out);
		sem_post(&shared_info->append_file);

		/*If restaurant is closed.Go home*/
		if (shared_info->restaurant_open == 0)
			break;

		/*try finding a table for this group*/
		if (group_size > 0)
		{
			best_table = -1;
			best_table_size = 100;/*something big.I dont know of a restaurant with such big tables..*/
			for (i=0;i<tables_num;i++)
			{
				if ( shared_tables[i].group_id == -1)
				{/*if table is empty*/
					table_size = shared_tables[i].capacity;
					if (table_size >= group_size)
					{/*if group can fit in this table*/
						if (table_size < best_table_size)
						{/*if we waste less space than with last empty table*/
							best_table = i;
							best_table_size = table_size;
						}
					}
				}
			}

			if (best_table >= 0)
			{
				sem_wait(&shared_info->append_file);
				fprintf(out, "\nDoorman found table %d for customer with size %d\n",best_table,group_size);
				fflush(out);
				sem_post(&shared_info->append_file);

				shared_info->doorman.answer = best_table;

				sem_wait(&shared_info->stats.stats_write);
				shared_info->stats.people_at_tables += group_size;
				sem_post(&shared_info->stats.stats_write);
			}
			else/*no table available*/
			{
				/*only one doorman,and only he checks people num at bar*/
				/*Also capacity doesnt change.No need for semaphore*/
				bar_space = shared_info->bar.capacity - shared_info->stats.people_at_bar; 
				
/*SOS--------------------------------------------------------------------------------------*/
				/*TO BE CONTINUED!ALWAYS GOES TO ELSE FOR NOW*/
				if (bar_space >= group_size && 1==0)
				{/*if bar has space for this group*/
					shared_info->doorman.answer = -1;
					shared_info->stats.people_at_bar += group_size;
					sem_wait(&shared_info->append_file);
					fprintf(out, "\nDoorman sending customer to bar\n");
					fflush(out);
					sem_post(&shared_info->append_file);
				}
				else
				{
					shared_info->doorman.answer = -2;
					sem_wait(&shared_info->stats.stats_write);
					shared_info->stats.groups_gone += 1;
					sem_post(&shared_info->stats.stats_write);	
					
					sem_wait(&shared_info->append_file);
					fprintf(out, "\nDoorman tells customer that there is no table and no room in the bar\n");
					fflush(out);
					sem_post(&shared_info->append_file);					
				}
			}
			sem_post(&shared_info->doorman.doorman_answer);/*deliver answer*/

		}
		else
		{
			sem_wait(&shared_info->append_file);
			fprintf(out, "\n<!>Doorman got a strange group_size.He is angry and leaving the restaurant!\n");
			fflush(out);
			sem_post(&shared_info->append_file);
			break;
		}

	}
	
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