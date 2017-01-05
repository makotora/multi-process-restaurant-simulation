#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "functions.h"

int intToString(int integer,char **string_ptr)
{
	int number_of_digits = 1;
	if (integer < 0) 
		integer*= -1;
	
	int tempint = integer;

	while (tempint > 9)
	{
		tempint /= 10;
		number_of_digits++;
	}
	
	*string_ptr = malloc( (number_of_digits+1)*sizeof(char));
	if (*string_ptr == NULL) return 10;

	sprintf(*string_ptr,"%d",integer);

	return 0;
}