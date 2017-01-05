#include <semaphore.h>

typedef struct table table;

struct table
{
	int group_id;/*for the doorman to set,and waiter to unset*/
	int group_activity;/*for the group to change,-1:None 0:ordering 1:eating 2:ready to pay*/
	int waiter_id;/*for the first waiter coming to set,and unset*/
	int capacity;/*initialized remains the same*/
};


typedef struct doorman_struct doorman_struct;

struct doorman_struct
{
	sem_t doorman_busy;
	int answer;
};

typedef struct door_struct door_struct;

struct door_struct
{
	sem_t door_queue;
	int group_size;
};

typedef struct bar_struct bar_struct;

struct bar_struct
{
	sem_t bar_queue;
	int group_size;
};

typedef struct tables_struct tables_struct;

struct tables_struct
{
	sem_t waiter_busy;
	sem_t waiter_queue;
};

typedef struct statistics statistics;

struct statistics
{
	sem_t stats_write;
	int people_at_tables;
	int people_at_bar;
	int groups_serviced;
	int groups_gone;
	int income;
	int groups_bored_waiting;
};

typedef struct shared_struct shared_struct;

struct shared_struct
{
	int tables_num;
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
