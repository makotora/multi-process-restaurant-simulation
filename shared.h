#ifndef SHARED_H
#define SHARED_H

#include <semaphore.h>

typedef struct table table;

struct table
{
	int group_id;/*for the doorman to set,and waiter to unset*/
	int group_size;/*for the doorman to set,and waiter to unset*/
	int group_activity;/*for the group and its waiter to change,-1:None 0:ready to order 1:eating 2:ready to pay*/
	int waiter_id;/*for the first waiter coming to set,and unset*/
	int income;
	int capacity;/*initialized remains the same*/
	sem_t table_service;
};


typedef struct doorman_struct doorman_struct;

struct doorman_struct
{
	sem_t doorman_busy;
	sem_t doorman_answer;
	sem_t doorman_bar_answer;
	int answer;
	int bar_answer;
};

typedef struct door_struct door_struct;

struct door_struct
{
	sem_t door_queue;
	sem_t door_group_size;
	int group_size;
};

typedef struct bar_struct bar_struct;

struct bar_struct
{
	sem_t bar_queue;
	sem_t bar_group_size;
	sem_t bar_group_bored;
	int groups_num;
	int group_size;
	int group_bored;
	int capacity;
};

typedef struct tables_struct tables_struct;

struct tables_struct
{
	sem_t waiter_busy;
	sem_t waiter_table;
	sem_t table_change;
	int free_tables;
};

typedef struct statistics statistics;

struct statistics
{
	sem_t stats_write;
	int people_at_tables;
	int people_at_bar;
	int bar_groups;
	int groups_serviced;
	int groups_gone;
	int door_groups_gone;
	int groups_bored_waiting;
	int groups_done;
	int income;
};

typedef struct shared_struct shared_struct;

struct shared_struct
{
	int tables_num;
	int restaurant_open;
	sem_t append_file;
	doorman_struct doorman;
	door_struct door;
	bar_struct bar;
	tables_struct tables;
	statistics stats;
};


void print_shared_struct(FILE * out,shared_struct * ss);
void print_stats(FILE * out,shared_struct * ss);
void print_shared_tables(FILE * out,table * tables, int tables_size);
void print_shared_tables_incomes(FILE * out, table * tables, int tables_num);
#endif