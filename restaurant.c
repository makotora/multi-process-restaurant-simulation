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
#include "functions.h"

#define MAX_GROUP_SIZE 8


int main(int argc, char const *argv[])
{

	int * table_capacities;
	int i,err,max_groups,closing_time,tables_num,bar_size,waiters,shared_id;
	char * config_name = NULL;
	char * append_name = NULL;
	FILE * config_file;
	FILE * append_file;
	FILE * out;
	max_groups = -1;
	closing_time = -1;

	if (argc < 7)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time [-a appendfile]\n");
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
				closing_time = atoi(argv[i+1]);
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
				printf("Usage: ./restaurant -n customers -l configfile -d time [-a appendfile]\n");
				return 2;
			}

		}
	}

	if (max_groups == -1)
	{
		printf("You didnt give the -n argument\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time [-a appendfile]\n");
		return 3;
	}
	else if (config_name == NULL)
	{
		printf("You didnt give the -l argument\n");	
		printf("Usage: ./restaurant -n customers -l configfile -d time [-a appendfile]\n");
		return 3;
	}
	else if (closing_time == -1)
	{
		printf("You didnt give the -d argument\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time [-a appendfile]\n");
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

	printf("Arguments given: n %d l %s d %d\n",max_groups,config_name,closing_time);

	config_file = fopen(config_name,"r");
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
	printf("Config file info read.\n");
	printf("tables %d\n",tables_num);
	for (i=0;i<tables_num;i++)
		printf("%d ",table_capacities[i]);
	printf("\nbar size %d\n", bar_size);
	printf("max waiters %d\n", waiters);
	

	printf("--INITIALISING TABLES--\n");
	table * tables;
	tables = malloc(tables_num*sizeof(table));
	for (i=0;i<tables_num;i++)
	{
		tables[i].group_id = -1;
		tables[i].group_activity = -1;
		tables[i].waiter_id = -1;
		tables[i].capacity = table_capacities[i];
		printf("TABLE %d: capacity is %d\n",i+1,tables[i].capacity);
	}
	printf("--DONE INITIALISING TABLES--\n\n");

	/*Create shared memory*/
	shared_id = shmget(IPC_PRIVATE,sizeof(shared_struct)+tables_num*sizeof(int),0666);
	if (shared_id == -1)
		perror("shmget");

	/*Initialise shared memory (semaphores too)*/

	void * shared_memory = (void *) shmat(shared_id, (void*)0, 0);
	if ( (int) shared_memory == -1)
		perror("shmat");

	shared_struct * shared_info = shared_memory;
	sem_init(&shared_info->append_file,1,1);
	shared_info->tables_num = tables_num;
	sem_init(&shared_info->doorman.doorman_busy,1,0);
	shared_info->doorman.answer = -3;
	sem_init(&shared_info->door.door_queue,1,1);
	shared_info->door.group_size = -1;
	sem_init(&shared_info->bar.bar_queue,1,1);
	shared_info->bar.group_size = -1;
	sem_init(&shared_info->tables.waiter_busy,1,0);
	sem_init(&shared_info->tables.waiter_queue,1,1);
	sem_init(&shared_info->stats.stats_write,1,1);
	shared_info->stats.people_at_tables = 0;
	shared_info->stats.people_at_bar = 0;
	shared_info->stats.groups_serviced = 0;
	shared_info->stats.groups_gone = 0;
	shared_info->stats.income = 0;
	shared_info->stats.groups_bored_waiting = 0;
	
	/*Copy tables array in shared memory (after sharedStruct)*/
	table * shared_tables = shared_memory + sizeof(shared_struct);
	memcpy(shared_tables,tables,tables_num*sizeof(table));
	
	/*START CREATING OTHER PROCESSES*/
	srand(time(NULL));
	char * doorman_max_time = "3";
	char * customer_max_time = "5";
	char customer_people[10];
	char id_string[20];
	sprintf(id_string,"%d",shared_id);

	/*Create doorman*/
	if (fork() == 0)
	{
		free(tables);
		free(table_capacities);
		
		if (append_file == NULL)
		{
			execl("./doorman","./doorman", "-d",doorman_max_time, "-s", id_string,NULL);
			perror("execl");
		}
		else
		{
			printf("%s %s %s\n",doorman_max_time,id_string,append_name);
			execl("./doorman","./doorman", "-d",doorman_max_time, "-s", id_string,"-a", append_name, NULL);
			perror("execl");
		}
	}

	sleep(3);
	/*Call customers*/
	for (i=0;i<max_groups;i++)
	{
		int group_size;
		group_size = ( rand() % MAX_GROUP_SIZE ) + 1;

		if (fork() == 0)
		{
			free(tables);
			free(table_capacities);

			sprintf(customer_people,"%d",group_size);
			
			if (append_file == NULL)
			{
				execl("./customer","./customer", "-n", customer_people,"-d",customer_max_time, "-s", id_string,NULL);
				perror("execl");
			}
			else
			{
				printf("%s %s %s %s\n",customer_people,customer_max_time,id_string,append_name);
				execl("./customer","./customer", "-n", customer_people,"-d",customer_max_time, "-s", id_string,"-a", append_name, NULL);
				perror("execl");
			}
		}
	}

	sem_wait(&shared_info->append_file);
	fprintf(out, "\n---Restaurant is now open---\n");
	print_shared_struct(out, shared_info);
	print_shared_tables(out, shared_tables, tables_num);
	fflush(out);
	sem_post(&shared_info->append_file);

	/*Wait for all customers to leave*/
	int pid;
	i = 0;
	while (i < max_groups)
	{
		pid = waitpid(-1,NULL,0);/*wait for any customer*/
		if (pid == -1) 
			continue;/*in case of an error message try again*/

		sem_wait(&shared_info->append_file);
		fprintf(out, "\nRestaurant: Customer %d left the restaurant\n",pid);
		fflush(out);
		sem_post(&shared_info->append_file);

		i++;
	}

	/*Call doorman so he can go home*/
	fflush(out);
	sem_wait(&shared_info->append_file);
	fprintf(out, "\nRestaurant calling doorman so he can go home\n");
	sem_post(&shared_info->doorman.doorman_busy);
	fflush(out);
	sem_post(&shared_info->append_file);

	pid = waitpid(-1,NULL,0);/*wait for doorman to leave the restaurant*/

	/*All children processes have ended.No reason to use a semaphore*/
	fprintf(out, "\nRestaurant closing!\n");
	fflush(out);

	sem_destroy(&shared_info->append_file);
	sem_destroy(&shared_info->doorman.doorman_busy);
	sem_destroy(&shared_info->door.door_queue);
	sem_destroy(&shared_info->bar.bar_queue);
	sem_destroy(&shared_info->tables.waiter_busy);
	sem_destroy(&shared_info->tables.waiter_queue);
	

	err = shmctl ( shared_id , IPC_RMID , 0) ;
	if ( err == -1) 
		perror ("shmctl");		
	
	free(tables);
	free(table_capacities);

	return 0;
}