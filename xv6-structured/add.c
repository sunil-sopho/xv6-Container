// add two number 

# include "types.h"
# include "user.h"
#include "date.h"
//#include <stdlib.h>

int
main ( int argc , char * argv [])
{
    // check if argc = 3
//    if (argc != 3){
//        printf(1,"usage add <num1> <num2>]n");
//    }

    // atoi to char array to integer
//    int num1 = atoi(argv[1]);
//    int num2 = atoi(argv[2]);
    int num1=3,num2=7;
    printf(1,"sum of num1 :%d and num2:%d is %d",num1,num2,add(num1,num2)) ;
    exit () ;
}


