#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

volatile int x; // "volatile" to forbid compiler optimization

#define DO_0 ++x;
#define DO_1 {DO_0; DO_0; DO_0; DO_0; DO_0; DO_0; DO_0; DO_0; DO_0; DO_0}
#define DO_2 {DO_1; DO_1; DO_1; DO_1; DO_1; DO_1; DO_1; DO_1; DO_1; DO_1}
#define DO_3 {DO_2; DO_2; DO_2; DO_2; DO_2; DO_2; DO_2; DO_2; DO_2; DO_2}
#define DO_4 {DO_3; DO_3; DO_3; DO_3; DO_3; DO_3; DO_3; DO_3; DO_3; DO_3}
#define DO_5 {DO_4; DO_4; DO_4; DO_4; DO_4; DO_4; DO_4; DO_4; DO_4; DO_4}
#define DO_6 {DO_5; DO_5; DO_5; DO_5; DO_5; DO_5; DO_5; DO_5; DO_5; DO_5}
#define DO_7 {DO_6; DO_6; DO_6; DO_6; DO_6; DO_6; DO_6; DO_6; DO_6; DO_6}
#define DO_8 {DO_7; DO_7; DO_7; DO_7; DO_7; DO_7; DO_7; DO_7; DO_7; DO_7}
#define DO_9 {DO_8; DO_8; DO_8; DO_8; DO_8; DO_8; DO_8; DO_8; DO_8; DO_8}

int main()
{
	printf("Starting Test BIG CODE SEGMENT...\n");
    DO_5; // do trivial stuff 1 million times
    printf("x is %d and should be 100000\n", x);
	if (x == 100000) 
			printf("Test passed\n");
	else
			printf("Test NOT passed..\n");
}
