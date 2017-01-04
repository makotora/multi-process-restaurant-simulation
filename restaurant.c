#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "shared.h"


int main(int argc, char const *argv[])
{

	int * table_capacities;
	int i,max_groups,closing_time,tables_num,bar_size,waiters,shared_id;
	char * config_name;
	FILE * config_file;

	if (argc != 7)
	{
		printf("Invalid number of arguments!\n");
		printf("Usage: ./restaurant -n customers -l configfile -d time\n");
		return 1;
	}
	else
	{
		i = 1;
		while (i < 7)
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
			else
			{
				printf("Invalid argument given: %s\n",argv[i]);
				printf("Usage: ./restaurant -n customers -l configfile -d time\n");
				return 2;
			}

		}
	}

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
	shared_struct * shared_info = shared_memory;
	shared_info->tables_num = tables_num;
	sem_init(&shared_info->doorman.doorman_busy,1,0);
	shared_info->doorman.answer = -3;
	sem_init(&shared_info->door.door_queue,1,1);
	shared_info->door.group_size = -1;
	sem_init(&shared_info->bar.bar_queue,1,1);
	shared_info->bar.group_size = -1;
	sem_init(&shared_info->tables.waiter_busy,1,0);
	sem_init(&shared_info->tables.waiter_queue,1,1);
	
	/*Copy tables array in shared memory (after sharedStruct)*/
	table * shared_tables = shared_memory + sizeof(shared_struct);
	memcpy(shared_tables,tables,tables_num*sizeof(table));
	print_shared_struct(shared_info);
	print_shared_tables(shared_tables, tables_num);

	free(tables);
	free(table_capacities);

	return 0;
}