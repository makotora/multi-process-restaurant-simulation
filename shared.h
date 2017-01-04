typedef struct table table;

struct table
{
	int group_id;/*for the doorman to set,and group to unset*/
	int group_activity;/*for the group to change,-1:None 0:ordering 1:eating 2:ready to pay*/
	int waiter_id;/*for the first waiter coming to set,and the customers to unset*/
	int capacity;/*initialized*/
};


typedef struct door_struct door_struct;
typedef struct bar_struct bar_struct;
typedef struct tables_struct tables_struct;
typedef struct shared_struct shared_struct;

struct shared_struct
{
	door_struct door;
	bar_struct bar;
	tables_struct tables;	
};


int tables_create(table ** table_ptr_ptr,int table_num,int * table_capacities);
int tables_free(table ** table_ptr_ptr);
