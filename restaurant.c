#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include "shared.h"

#define MAX_GROUP_SIZE 8


int main(int argc, char const *argv[])
{

	int * table_capacities;
	int i,err,max_groups,stats_time,tables_num,bar_size,waiters,shared_id,open_time,check_time;
	int doorman_no_prints = 0,
		waiter_no_prints = 0,
	   	customer_no_prints = 0;

	char * config_name = NULL;
	char * append_name = NULL;
	FILE * config_file;
	FILE * append_file;
	FILE * out;
	max_groups = -1;
	stats_time = -1;

	if (argc < 9)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
		return 1;
	}
	else
	{
		i = 1;
		while (i < argc)
		{
			if ( !strcmp(argv[i],"-n"))
			{
				max_groups = atoi(argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-l"))
			{
				config_name = malloc( (strlen(argv[i+1]) + 1) * sizeof(char) );
				strcpy(config_name,argv[i+1]);
				i += 2;
			}
			else if ( !strcmp(argv[i],"-d"))
			{
				stats_time = atoi(argv[i+1]);
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
				printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
				return 2;
			}

		}
	}

	if (max_groups == -1)
	{
		printf("You didnt give the -n argument\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
		return 3;
	}
	else if (config_name == NULL)
	{
		printf("You didnt give the -l argument\n");	
		printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
		return 3;
	}
	else if (stats_time == -1)
	{
		printf("You didnt give the -d argument\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
		return 3;
	}
	else if (append_name == NULL)
	{
		printf("You didnt give the -a argument\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time -a appendfile\n");
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

	config_file = fopen(config_name,"r");
	free(config_name);
	if (config_file == NULL)
	{
		printf("Error opening configuration file '%s'!\n",config_name);
		return 3;
	}

	char line[100];
	char * token;
	
	fgets(line,100, config_file);
	token = strtok(line," ");
	token = strtok(NULL," "); 

	tables_num = atoi(token);
	table_capacities = malloc(tables_num*sizeof(int));

	fgets(line,100, config_file);
	token = strtok(line," ");
	for (i=0;i<tables_num;i++)
	{
		token = strtok(NULL," ");
		table_capacities[i] = atoi( token );
	}

	fgets(line,100, config_file);
	token = strtok(line," ");
	token = strtok(NULL," ");
	bar_size = atoi( token );

	fgets(line,100, config_file);
	token = strtok(line," ");
	token = strtok(NULL," ");
	waiters = atoi( token );

	fclose(config_file);

	table * tables;
	tables = malloc(tables_num*sizeof(table));
	for (i=0;i<tables_num;i++)
	{
		tables[i].group_id = -1;
		tables[i].group_size = -1;
		tables[i].group_activity = -1;
		tables[i].waiter_id = -1;
		tables[i].income = 0;
		tables[i].capacity = table_capacities[i];
		sem_init(&(tables[i].table_service),1,0);
	}
	free(table_capacities);/*only needed this to initialise*/

	/*Create shared memory*/
	shared_id = shmget(IPC_PRIVATE,sizeof(shared_struct)+tables_num*sizeof(table),0666);
	if (shared_id == -1)
		perror("shmget");

	/*Initialise shared memory (semaphores too)*/

	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (long int) shared_memory == -1)
		perror("shmat");

	shared_struct * shared_info = shared_memory;
	shared_info->tables_num = tables_num;
	shared_info->restaurant_open = 1;
	sem_init(&shared_info->append_file,1,1);
	
	sem_init(&shared_info->doorman.doorman_busy,1,0);
	sem_init(&shared_info->doorman.doorman_answer,1,0);
	sem_init(&shared_info->doorman.doorman_bar_answer,1,0);
	shared_info->doorman.answer = -3;
	shared_info->doorman.bar_answer = -3;
	
	sem_init(&shared_info->door.door_queue,1,1);
	sem_init(&shared_info->door.door_group_size,1,0);
	shared_info->door.group_size = -1;
	
	sem_init(&shared_info->bar.bar_queue,1,1);
	sem_init(&shared_info->bar.bar_group_size,1,0);
	sem_init(&shared_info->bar.bar_group_bored,1,0);
	shared_info->bar.groups_num = 0;
	shared_info->bar.group_size = -1;
	shared_info->bar.group_bored = -1;
	shared_info->bar.capacity = bar_size;
	
	sem_init(&shared_info->tables.waiter_busy,1,0);
	sem_init(&shared_info->tables.waiter_table,1,1);
	sem_init(&shared_info->tables.table_change,1,1);
	shared_info->tables.free_tables = tables_num;

	sem_init(&shared_info->stats.stats_write,1,1);
	shared_info->stats.people_at_tables = 0;
	shared_info->stats.people_at_bar = 0;
	shared_info->stats.bar_groups = 0;
	shared_info->stats.groups_serviced = 0;
	shared_info->stats.groups_gone = 0;
	shared_info->stats.door_groups_gone = 0;
	shared_info->stats.groups_bored_waiting = 0;
	shared_info->stats.groups_done = 0;
	shared_info->stats.income = 0;
	
	/*Copy tables array in shared memory (after sharedStruct)*/
	table * shared_tables = shared_memory + sizeof(shared_struct);
	memcpy(shared_tables,tables,tables_num*sizeof(table));
	/*free local tables copy,we only wanted it for the memcpy*/
	for (i=0;i<tables_num;i++)
		sem_destroy(&(tables[i].table_service));
	free(tables);
	

	sem_wait(&shared_info->append_file);
	fprintf(out, "\n---Restaurant is now open---\n");
	fflush(out);
	sem_post(&shared_info->append_file);
	open_time = time(NULL);

	/*START CREATING OTHER PROCESSES*/
	srand(time(NULL));
	char * doorman_max_time = "3";
	char id_string[20];
	sprintf(id_string,"%d",shared_id);


	/*Create doorman*/
	if (fork() == 0)
	{
		if (doorman_no_prints || append_file == NULL)
		{
			execl("./doorman","./doorman", "-d",doorman_max_time, "-s", id_string,NULL);
			perror("execl");
		}
		else
		{
			execl("./doorman","./doorman", "-d",doorman_max_time, "-s", id_string,"-a", append_name, NULL);
			perror("execl");
		}
	}

	/*Call waiters*/
	char * waiter_max_time = "5";
	char * waiter_moneyamount = "20";
	for (i=0;i<waiters;i++)
	{

		if (fork() == 0)
		{
			if (waiter_no_prints || append_file == NULL)
			{
				execl("./waiter","./waiter", "-d", waiter_max_time,"-m", waiter_moneyamount, "-s", id_string,NULL);
				perror("execl");
			}
			else
			{
				execl("./waiter","./waiter", "-d", waiter_max_time,"-m", waiter_moneyamount, "-s", id_string,"-a", append_name, NULL);
				perror("execl");
			}
		}
	}

	/*Call customers*/
	char * customer_max_time = "1";
	char customer_people[10];
	for (i=0;i<max_groups;i++)
	{
		int group_size;
		group_size = ( rand() % MAX_GROUP_SIZE ) + 1;

		if (fork() == 0)
		{
			sprintf(customer_people,"%d",group_size);
			
			if (customer_no_prints || append_file == NULL)
			{
				execl("./customer","./customer", "-n", customer_people,"-d",customer_max_time, "-s", id_string,NULL);
				perror("execl");
			}
			else
			{
				execl("./customer","./customer", "-n", customer_people,"-d",customer_max_time, "-s", id_string,"-a", append_name, NULL);
				perror("execl");
			}
		}
	}

	int groups_done = 0;
	while (groups_done != max_groups)
	{
		sleep(stats_time);
		/*print stats*/
		sem_wait(&shared_info->stats.stats_write);
		
		print_stats(stdout ,shared_info);
		print_shared_tables_incomes(stdout, shared_tables, tables_num);
		fflush(stdout);
		sem_wait(&shared_info->append_file);
		print_stats(out ,shared_info);
		print_shared_tables_incomes(out, shared_tables, tables_num);
		fflush(out);
		sem_post(&shared_info->append_file);
		groups_done = shared_info->stats.groups_done; 
		
		sem_post(&shared_info->stats.stats_write);

	}


	/*Wait for all customers to leave*/
	int pid;
	i = 0;
	while (i < max_groups)
	{
		pid = waitpid(-1,NULL,0);/*wait for any customer*/
		if (pid == -1) 
			continue;/*in case of an error message try again*/

		sem_wait(&shared_info->append_file);
		fprintf(out, "\n---Restaurant: Customer %d left the restaurant---\n",pid);
		fflush(out);
		sem_post(&shared_info->append_file);

		i++;
	}

	/*Call doorman and waiters so they can go home*/
	shared_info->restaurant_open = 0;

	sem_wait(&shared_info->append_file);
	fprintf(out, "\n---Restaurant calling doorman so he can go home---\n");
	fflush(out);
	sem_post(&shared_info->append_file);

	sem_post(&shared_info->doorman.doorman_busy);
	waitpid(-1,NULL,0);/*wait for doorman to leave*/
	sem_wait(&shared_info->append_file);
	fprintf(out, "\n---Restaurant: Doorman left the restaurant---\n");
	fflush(out);
	sem_post(&shared_info->append_file);

	sem_wait(&shared_info->append_file);
	fprintf(out, "\n---Restaurant calling waiters so they can go home---\n");
	fflush(out);
	sem_post(&shared_info->append_file);

	for (i=0;i<waiters;i++)
		sem_post(&shared_info->tables.waiter_busy);

	for (i=0;i<waiters;i++)
	{
		pid = waitpid(-1,NULL,0);/*wait for waiters to leave*/
		sem_wait(&shared_info->append_file);
		fprintf(out, "\n---Restaurant: Waiter %d left the restaurant---\n",pid);
		fflush(out);
		sem_post(&shared_info->append_file);
	}

	/*All children processes have ended.No reason to use a semaphore*/
	check_time = time(NULL);
	fprintf(out, "\n---Restaurant closing!Time elapsed %d---\n",check_time-open_time);
	fprintf(stdout, "\n---Restaurant closing!Time elapsed %d---\n",check_time-open_time);
	print_stats(out,shared_info);
	print_stats(stdout,shared_info);
	fflush(out);
	sem_destroy(&shared_info->append_file);
	sem_destroy(&shared_info->doorman.doorman_busy);
	sem_destroy(&shared_info->doorman.doorman_answer);
	sem_destroy(&shared_info->doorman.doorman_bar_answer);
	sem_destroy(&shared_info->door.door_queue);
	sem_destroy(&shared_info->door.door_group_size);
	sem_destroy(&shared_info->bar.bar_queue);
	sem_destroy(&shared_info->bar.bar_group_size);
	sem_destroy(&shared_info->bar.bar_group_bored);
	sem_destroy(&shared_info->tables.waiter_busy);
	sem_destroy(&shared_info->tables.waiter_table);
	sem_destroy(&shared_info->tables.table_change);
	for (i=0;i<tables_num;i++)
		sem_destroy(&(shared_tables[i].table_service));
	

	err = shmctl ( shared_id , IPC_RMID , 0) ;
	if ( err == -1) 
		perror ("shmctl");

	if (append_name != NULL)
		free(append_name);
	
	fclose(out);
	
	return 0;
}