#include"process.h"

char buff[SUM];
static int count = 0;
const char* s = "|/-\\";

void process(double progress,double data)
{
    memset(buff,0,SUM);
    const int len = strlen(s);

    double pgs = progress * 100 / data;

    for(int i = 0;i < (int)pgs;i++){
        buff[i] = SIZE;
    }

    printf("[%-100s][%-2.1f%%][%c]\r",buff,pgs,s[count % len]);
    fflush(stdout);

    count++;
}