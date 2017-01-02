typedef struct group group;

struct group
{
	int group_id;
	group * next_group;
};


struct queue
{
	group * first_group;
	group * last_group;
};

typedef struct queue queue;

int queue_create(queue ** queue_ptr_ptr);
int queue_free(queue ** queue_ptr_ptr);
int queue_push(queue * queue_ptr,int group_id);
int queue_pop(queue * queue_ptr);
