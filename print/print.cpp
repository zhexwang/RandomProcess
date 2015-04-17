#include "type.h"
#include <stdlib.h>
#include <cstdio>

#define LEN 50

static void sharp(INT32 done, INT32 left)
{
	for (INT32 idx=0; idx<done; idx++)
		putchar('#');
	for (INT32 idx=0; idx<left; idx++)
		putchar(' ');
}

void progress_begin()
{
	for (INT32 k=0; k<(LEN+4); k++)
		putchar(' ');
}
void print_progress(INT32 current, INT32 max)
{
	//clear
	for (INT32 k=0; k<(LEN+4); k++)
		putchar('\b');
	
	//print progress
	INT32 progress_done = LEN*current/max;
	INT32 progress_left = LEN-progress_done;
	sharp(progress_done, progress_left);
	//print percent
	fprintf(stdout, "%3d%%", 100*current/max);
	fflush(stdout); //output message on time, please.
}
void progress_end()
{
	fprintf(stdout, "\n");
}
