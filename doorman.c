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

int find_best_table(table * tables, int tables_num, int group_size)
{
	int i,best_table = -1;
	int best_table_size = 100;/*something big.I dont know of a restaurant with such big tables..*/
	for (i=0;i<tables_num;i++)
	{
		int table_size;
		if ( tables[i].group_id == -1)
		{/*if table is empty*/
			table_size = tables[i].capacity;
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

	return best_table;
}


int main(int argc, char const *argv[])
{
	int i,err,max_period,shared_id;
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
		out = stdout;
	}

	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (long int) shared_memory == -1)
	{
		perror("shmat");
	}

	shared_struct * shared_info = shared_memory;
	table * shared_tables = shared_memory + sizeof(shared_struct);
	int tables_num = shared_info->tables_num;
	table * current_tables = malloc(tables_num*sizeof(table));
	
	sem_wait(&shared_info->append_file);
	fprintf(out, "\n|.| Doorman arrived with max_period %d and shmdid %d\n",max_period,shared_id);
	fflush(out);
	sem_post(&shared_info->append_file);

	srand(time(NULL));

	while (1)
	{/*when doorman has nothing to do its closing time and we break*/
		/*wait for someone to call*/
		int group_size,best_table,bar_space,groups_in_bar,free_tables,bored_group;
		int action_period;

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n|.| Doorman is waiting for someone to call him\n");
		fflush(out);	
		sem_post(&shared_info->append_file);

		sem_wait(&shared_info->doorman.doorman_busy);

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n|.| Doorman was called by someone\n");
		fflush(out);
		sem_post(&shared_info->append_file);

		/*If restaurant is closed.Go home*/
		if (shared_info->restaurant_open == 0)
			break;

		/*GET A COPY OF THE CURRENT STATE OF TABLES*/
		sem_wait(&shared_info->tables.table_change);
		free_tables = shared_info->tables.free_tables;
		memcpy(current_tables, shared_tables, tables_num*sizeof(table));
		sem_post(&shared_info->tables.table_change);
		
		/*this is used only for finding a table*/
		/*THIS IS DONE SO WE MAKE SURE THAT WE ARE FAIR WITH PEOPLE AT BAR*/
		/*A SUDDEN FREE OF TABLE COULD OTHERWISE LEAD TO:*/
		/*a)group in door go inside instead of group in bar,b)a group with lower priority in bar go inside*/

		/*First look at bar if (there is a free table and there are actually people in the bar)*/
		groups_in_bar = shared_info->bar.groups_num;

		if ( groups_in_bar > 0)
		{/*if there are people in the bar we need to let them know if there is a table because they are calling*/
			sem_wait(&shared_info->append_file);
			fprintf(out, "\n\t|.| Doorman is checking the bar\n");
			fflush(out);
			sem_post(&shared_info->append_file);

			/*first check if you can give a table the first group in the bar (the one that posted)*/
			sem_wait(&shared_info->bar.bar_group_size);/*wait for him to tell you its size*/
			action_period = ( rand() % max_period ) + 1;

			group_size = shared_info->bar.group_size;

			/*look how many free tables there are*/

			if (free_tables > 0)
				best_table = find_best_table(current_tables,tables_num,group_size);
			else
				best_table = -1;

			if (best_table >= 0)
			{
				sem_wait(&shared_info->append_file);
				fprintf(out, "\n\t|.| Doorman found table %d for customer in bar (size %d)n",best_table,group_size);
				fflush(out);
				sem_post(&shared_info->append_file);

				shared_info->doorman.bar_answer = best_table;
				shared_info->bar.groups_num--;
				current_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
				/*no reason for more info at this local table*/
				shared_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
				shared_tables[best_table].group_size = group_size;/*Note the size of the group about to sit*/

				sleep(action_period);
				sem_post(&shared_info->doorman.doorman_bar_answer);/*deliver answer*/

				sem_wait(&shared_info->stats.stats_write);
				shared_info->stats.people_at_tables += group_size;
				shared_info->stats.people_at_bar -= group_size;
				sem_post(&shared_info->stats.stats_write);

				free_tables--;

				sem_wait(&shared_info->tables.table_change);
				shared_info->tables.free_tables--;
				sem_post(&shared_info->tables.table_change);
			}
			else/*no table available*/
			{
				sem_wait(&shared_info->append_file);
				fprintf(out, "\n\t|.| Doorman didnt find a table for customer in bar (size %d)\n",group_size);
				fflush(out);
				sem_post(&shared_info->append_file);
				shared_info->doorman.bar_answer = -1;/*tell customer to keep waiting*/
				sleep(action_period);
				sem_post(&shared_info->doorman.doorman_bar_answer);/*deliver answer*/

				sem_wait(&shared_info->bar.bar_group_bored);/*wait to see if he is bored or not*/
				bored_group = shared_info->bar.group_bored;
				if ( bored_group )
				{/*if that group is bored waiting.note that he left*/
					sem_wait(&shared_info->append_file);
					fprintf(out, "\n\t|.| Doorman saw that customer in bar (size %d) got bored and left\n",group_size);
					fflush(out);
					sem_post(&shared_info->append_file);
					
					shared_info->bar.groups_num--;

					sem_wait(&shared_info->stats.stats_write);
					shared_info->stats.people_at_bar -= group_size;
					shared_info->stats.groups_bored_waiting++;
					sem_post(&shared_info->stats.stats_write);
				}

			}


			/*let he rest of the people in the bar know if they can now sit or not*/
			for (i=1;i<groups_in_bar;i++)
			{
				sem_wait(&shared_info->doorman.doorman_busy);/*wait for next group in bar to come*/
				sem_wait(&shared_info->bar.bar_group_size);/*wait for him to tell you its size*/
				action_period = ( rand() % max_period ) + 1;

				group_size = shared_info->bar.group_size;
					
				if (free_tables > 0)
					best_table = find_best_table(current_tables,tables_num,group_size);
				else
					best_table = -1;

				if (best_table >= 0)
				{
					sem_wait(&shared_info->append_file);
					fprintf(out, "\n\t|.| Doorman found table %d for customer in bar (size %d)\n",best_table,group_size);
					fflush(out);
					sem_post(&shared_info->append_file);

					shared_info->doorman.bar_answer = best_table;
					shared_info->bar.groups_num--;
					current_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
					/*no reason for more info at this local table*/
					shared_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
					shared_tables[best_table].group_size = group_size;/*Note the size of the group about to sit*/

					sleep(action_period);
					sem_post(&shared_info->doorman.doorman_bar_answer);/*deliver answer*/

					sem_wait(&shared_info->stats.stats_write);
					shared_info->stats.people_at_tables += group_size;
					shared_info->stats.people_at_bar -= group_size;
					sem_post(&shared_info->stats.stats_write);

					free_tables--;

					sem_wait(&shared_info->tables.table_change);
					shared_info->tables.free_tables--;
					sem_post(&shared_info->tables.table_change);
				}
				else/*no table available*/
				{
					sem_wait(&shared_info->append_file);
					fprintf(out, "\n\t|.| Doorman didnt find a table for customer in bar (size %d)\n",group_size);
					fflush(out);
					sem_post(&shared_info->append_file);
					shared_info->doorman.bar_answer = -1;/*tell customer to keep waiting*/
					sleep(action_period);
					sem_post(&shared_info->doorman.doorman_bar_answer);/*deliver answer*/

					sem_wait(&shared_info->bar.bar_group_bored);/*wait to see if he is bored or not*/
					bored_group = shared_info->bar.group_bored;
					if ( bored_group )
					{/*if that group is bored waiting.note that he left*/
						sem_wait(&shared_info->append_file);
						fprintf(out, "\n\t|.| Doorman saw that customer in bar (size %d) got bored and left\n",group_size);
						fflush(out);
						sem_post(&shared_info->append_file);
						
						shared_info->bar.groups_num--;

						sem_wait(&shared_info->stats.stats_write);
						shared_info->stats.people_at_bar -= group_size;
						shared_info->stats.groups_bored_waiting++;
						sem_post(&shared_info->stats.stats_write);
					}
				}
			}

		}
		
		/*if there is noone at the bar then someone called me at the door*/
		if ( groups_in_bar == 0)
			sem_wait(&shared_info->door.door_group_size);/*note that you saw his size*/
		else if ( sem_trywait(&shared_info->door.door_group_size) == 0 )
		/*if someone is at the bar but there is someone at the door as well,go respond to the door too*/
			sem_wait(&shared_info->doorman.doorman_busy);/*not that you saw him as well (his size was seen with trywait)*/
		else /*someone at the bar (answered) but noone at the door.We are done for now*/
			continue;

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n|.| Doorman is looking at the door\n");
		fflush(out);
		sem_post(&shared_info->append_file);
		action_period = ( rand() % max_period ) + 1;

		/*try finding a table for the next group waiting at the door*/
		/*see his size(its there since we did trywait)*/
		group_size = shared_info->door.group_size;

		if (free_tables > 0)
			best_table = find_best_table(current_tables,tables_num,group_size);
		else
			best_table = -1;

		if (best_table >= 0)
		{
			sem_wait(&shared_info->append_file);
			fprintf(out, "\n|.| Doorman found table %d for customer (size %d)\n",best_table,group_size);
			fflush(out);
			sem_post(&shared_info->append_file);

			shared_info->doorman.answer = best_table;
			current_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
			/*no reason for more info at this local table*/
			shared_tables[best_table].group_id = -2;/*Note that someone is about to sit there*/
			shared_tables[best_table].group_size = group_size;/*Note the size of the group about to sit*/

			sem_wait(&shared_info->stats.stats_write);
			shared_info->stats.people_at_tables += group_size;
			sem_post(&shared_info->stats.stats_write);

			free_tables--;

			sem_wait(&shared_info->tables.table_change);/*note that a table was taken*/
			shared_info->tables.free_tables--;
			sem_post(&shared_info->tables.table_change);

		}
		else/*no table available*/
		{
			/*only one doorman,and only he checks people num at bar*/
			/*Also capacity doesnt change.No need for semaphore*/
			bar_space = shared_info->bar.capacity - shared_info->stats.people_at_bar; 
			
			if (bar_space >= group_size)
			{/*if bar has space for this group*/
				shared_info->doorman.answer = -1;

				shared_info->stats.bar_groups++;
				shared_info->stats.people_at_bar += group_size;

				shared_info->bar.groups_num++;

				sem_wait(&shared_info->append_file);
				fprintf(out, "\n|.| Doorman sending customer (size %d) to bar\n",group_size);
				fflush(out);
				sem_post(&shared_info->append_file);
			}
			else
			{
				shared_info->doorman.answer = -2;
				sem_wait(&shared_info->stats.stats_write);
				shared_info->stats.door_groups_gone++;
				sem_post(&shared_info->stats.stats_write);	
				
				sem_wait(&shared_info->append_file);
				fprintf(out, "\n|.| Doorman tells customer (size %d) that there is no table and no room in the bar\n",group_size);
				fflush(out);
				sem_post(&shared_info->append_file);					
			}
		}
		sleep(action_period);
		sem_post(&shared_info->doorman.doorman_answer);/*deliver answer*/
	}
	
	sem_wait(&shared_info->append_file);
	fprintf(out, "\n|.| Doorman leaving the restaurant\n");
	fflush(out);
	sem_post(&shared_info->append_file);

	free(current_tables);
	
	err = shmdt (( void *) shared_memory ) ; /* Detach segment */
	if ( err == -1)
	{
		perror ("shmdt") ;
	}

	fclose(out);

	return 0;
}