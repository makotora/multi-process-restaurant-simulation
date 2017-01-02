#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

int queue_create(queue ** queue_ptr_ptr)
{
	queue * new_queue;
	new_queue = malloc(sizeof(queue));
	if ( new_queue == NULL ) 
		return 10;

	new_queue->first_group = NULL;
	new_queue->last_group = NULL;

	*queue_ptr_ptr = new_queue;

	return 0;
}


int queue_free(queue ** queue_ptr_ptr)
{
	queue * queue_ptr = *queue_ptr_ptr;
	group * current_group = queue_ptr->first_group;

	while (current_group != NULL)
	{
		group * next_group;
		next_group = current_group->next_group;

		free(current_group);
		current_group = next_group;
	}

	free(*queue_ptr_ptr);

	return 0;
}


int queue_push(queue * queue_ptr,int group_id)
{
	group * new_group = malloc(sizeof(group));
	if ( new_group == NULL )
		return 10;

	new_group->group_id = group_id;
	new_group->next_group = NULL;

	if ( queue_ptr->first_group == NULL )
	{
		queue_ptr->first_group = new_group;
		queue_ptr->last_group = new_group;
	}
	else
	{
		queue_ptr->last_group->next_group = new_group;
		queue_ptr->last_group = new_group;
	}

	return 0;
}


int queue_pop(queue * queue_ptr)
{
	if ( queue_ptr->first_group == NULL)
		return -1;
	else
	{
		group * popped_group = queue_ptr->first_group;
		int group_id = popped_group->group_id;

		queue_ptr->first_group = queue_ptr->first_group->next_group;
		free(popped_group);

		return group_id;
	}
}