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

#define MAX_PATIENCE 20

int main(int argc, char const *argv[])
{
	int i,err,people,max_period,eat_time,shared_id,pid,patience,bar_arrival;
	char * append_name = NULL;
	FILE * append_file = NULL;
	FILE * out;
	people = -1;
	max_period = -1;
	shared_id = -1;

	if (argc < 7)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./customer -n people -d period -s shmid [-a appendfile]\n");
		return 1;
	}
	else
	{
		i = 1;
		while (i < argc)
		{
			if ( !strcmp(argv[i],"-n"))
			{
				people = atoi(argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-d"))
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
				printf("Usage: ./customer -n people -d period -s shmid [-a appendfile]\n");
				return 2;
			}

		}
	}

	if (people == -1)
	{
		printf("You didnt give the -n argument\n");
		printf("Usage: ./customer -n people -d period -s shmid [-a appendfile]\n");
		return 3;
	}
	else if (max_period == -1)
	{
		printf("You didnt give the -d argument\n");
		printf("Usage: ./customer -n people -d period -s shmid [-a appendfile]\n");
		return 3;
	}
	else if (shared_id == -1)
	{
		printf("You didnt give the -s argument\n");
		printf("Usage: ./customer -n people -d period -s shmid [-a appendfile]\n");
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
		out = stdout;
	}

	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (long int) shared_memory == -1)
	{
		perror("shmat");
	}

	shared_struct * shared_info = shared_memory;
	table * shared_tables = shared_memory + sizeof(shared_struct);
	
	pid = getpid();
	srand(time(NULL));	
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nCustomer %d arrived with %d people max_period %d and shmdid %d\n",pid,people,max_period,shared_id);
	fflush(out);
	sem_post(&shared_info->append_file);

	/*wait at line,state size,call doorman*/
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nCustomer %d waiting at queue\n",pid);
	fflush(out);
	sem_post(&shared_info->append_file);
	
	sem_wait(&shared_info->door.door_queue);
	
	shared_info->door.group_size = people;
	
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nCustomer %d told doorman that his size is %d.Waiting for doorman's answer\n",pid,people);
	fflush(out);
	sem_post(&shared_info->append_file);

	sem_post(&shared_info->door.door_group_size);/*state your size*/
	sem_post(&shared_info->doorman.doorman_busy);/*call doorman*/

	/*Wait for an answer from doorman and let the next customer to ask doorman as well*/
	sem_wait(&shared_info->doorman.doorman_answer);

	int answer = shared_info->doorman.answer;
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nCustomer %d (size %d) got %d as answer and is leaving the queue\n",pid,people,answer);
	fflush(out);
	sem_post(&shared_info->append_file);

	shared_info->door.group_size = -1;/*reset group_size to -1 and leave queue*/
	sem_post(&shared_info->door.door_queue);
	int in_restaurant;
	table * my_table;

	if (answer >= 0)
	{
		in_restaurant = 1;/*not that we didnt leave*/
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nCustomer %d (size %d) is going to table %d\n",pid,people,answer);
		fflush(out);
		sem_post(&shared_info->append_file);

	}
	else if (answer == -1)
	{
		in_restaurant = 1;/*not that we didnt leave*/
		patience = ( rand() % MAX_PATIENCE ) + 1;/*My patience..I wont be here for ever*/
		bar_arrival = time(NULL);
		
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nCustomer %d (size %d) is going to the bar\n",pid,people);
		fflush(out);
		sem_post(&shared_info->append_file);
		
		while (answer == -1)
		{
			int bar_answer_time,waiting_time;

			sem_wait(&shared_info->bar.bar_queue);
			
			shared_info->bar.group_size = people;

			sem_wait(&shared_info->append_file);
			fprintf(out, "\n\tBar Customer %d told doorman that his size is %d.Waiting for doorman's answer\n",pid,people);
			fflush(out);

			sem_post(&shared_info->append_file);
			sem_post(&shared_info->bar.bar_group_size);/*state your size*/
			sem_post(&shared_info->doorman.doorman_busy);/*call doorman*/

			
			/*Wait for an answer from doorman and let the next customer to ask doorman as well*/
			sem_wait(&shared_info->doorman.doorman_bar_answer);
			bar_answer_time = time(NULL);
			waiting_time = bar_answer_time - bar_arrival;

			answer = shared_info->doorman.bar_answer;
			sem_wait(&shared_info->append_file);
			fprintf(out, "\n\tBar Customer %d (size %d) got %d as answer\n",pid,people,answer);
			fflush(out);
			sem_post(&shared_info->append_file);

			if (answer == -1)
			{
				if ( waiting_time > patience)/*if I have no more patience to wait*/
				{
					shared_info->bar.group_bored = 1;/*bored*/
					in_restaurant = 0;/*leave the restaurant*/
					
					sem_wait(&shared_info->append_file);
					fprintf(out, "\n\tBar Customer %d (size %d) is leaving because he is bored waiting\n",pid,people);
					fflush(out);
					sem_post(&shared_info->append_file);
					sem_post(&shared_info->bar.bar_group_bored);/*state that your are bored*/
					sem_post(&shared_info->bar.bar_queue);/*leave the bar queue*/					
					break;
				}

				shared_info->bar.group_bored = 0;
				sem_post(&shared_info->bar.bar_group_bored);
			}

			sem_post(&shared_info->bar.bar_queue);
		}

	}
	else if (answer == -2)
	{
		in_restaurant = 0;/*not that we are leaving the restaurant*/
		sem_wait(&shared_info->append_file);
		fprintf(out, "\nCustomer %d (size %d) is leaving because restaurant is full\n",pid,people);
		fflush(out);
		sem_post(&shared_info->append_file);
	}
	else
	{/*this should never happen*/
		in_restaurant = 0;
		sem_wait(&shared_info->append_file);
		fprintf(out, "\n<!>Customer %d got a strange answer.He is angry and leaving the restaurant!\n",pid);
		fflush(out);
		sem_post(&shared_info->append_file);
	}

	if (in_restaurant)
	{/*if we didnt leave,we are at a table*/
		my_table = &(shared_tables[answer]);

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n\t\tTable Customer %d (size %d) is now at table %d\n",pid,people,answer);
		fflush(out);
		sem_post(&shared_info->append_file);
		
		my_table->group_id = pid; /*sit at your table*/

		sem_post(&shared_info->tables.waiter_busy);/*make waiters notice that you need a waiter*/
		/*NO NEED TO WAIT FOR A WAITER TO COME SO YOU CAN ORDER,THE MENUS ARE IN FRONT THE TABLE*/
		/*THE WAITER WILL NOTE THAT YOU ARE HIS TABLE*/


		sem_wait(&shared_info->append_file);
		fprintf(out, "\n\t\tTable Customer %d (table %d) wants to order\n",pid,answer);
		fflush(out);
		sem_post(&shared_info->append_file);
		
		my_table->group_activity = 0;/*ready to order*/
		
		sem_post(&shared_info->tables.waiter_busy);/*request waiter for order*/

		
		sem_wait(&(my_table->table_service));/*wait for a waiter to your take order*/

		eat_time = (rand() % max_period) + 1;
		sleep(eat_time);/*eat + talk*/

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n\t\tTable Customer %d (table %d) wants to pay\n",pid,answer);
		fflush(out);
		sem_post(&shared_info->append_file);

		my_table->group_activity = 2;/*ready to pay*/
		
		sem_post(&shared_info->tables.waiter_busy);/*request waiter to pay*/
		
		sem_wait(&(my_table->table_service));/*wait for a waiter so you can pay and not go to jail*/

	}




	sem_wait(&shared_info->append_file);
	fprintf(out, "\nCustomer %d leaving the restaurant\n",pid);
	fflush(out);
	sem_post(&shared_info->append_file);

	sem_wait(&shared_info->stats.stats_write);
	shared_info->stats.groups_done++;
	sem_post(&shared_info->stats.stats_write);
	
	err = shmdt (( void *) shared_memory ) ; /* Detach segment */
	if ( err == -1)
	{ 
		perror ("shmdt") ;
	}

	fclose(out);

	return 0;

}