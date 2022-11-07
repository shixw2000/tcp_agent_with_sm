#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"testcase.h"


void usage(int argc, char* argv[]) {
    printf("usage: %s <opt>|\n", argv[0]);
}

int main(int argc, char* argv[]) {
    int ret = 0;    
        
    if (2 <= argc) {
        ret = test_main(argc, argv);
    } else {
        usage(argc, argv);
        return -1;
    }
    
    return ret;
}

