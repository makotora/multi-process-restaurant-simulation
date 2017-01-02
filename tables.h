typedef struct table table;

struct table
{
	int group_id;/*for the doorman to set,and group to unset*/
	int group_activity;/*for the group to change,0:ordering 1:eating 2:ready to pay*/
	int waiter_id;/*for the first waiter coming to set,and the customers to unset*/
	int capacity;/*initialized*/
};


int tables_create(table ** table_ptr_ptr,int table_num,int * table_capacities);
int tables_free(table ** table_ptr_ptr);
