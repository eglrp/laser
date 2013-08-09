#include "../hd_test_config.h"
#ifdef TEST_ENDIAN

#include <stdio.h>

union 
{
	int number;
	char s;
}test;

int testBigEndin()
{
	test.number=0x01000002;
    return (test.s==0x01);
}

void main()
{
    if (testBigEndin())	 
		printf("big\r\n");
	else 
		printf("small\r\n");	
}

#endif
