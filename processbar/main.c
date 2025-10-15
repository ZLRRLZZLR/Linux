#include"process.h"
#include<stdio.h>
#include<unistd.h>

double data = 1024.0;
double speed = 1.0;

typedef void (*callback_t)(double progress,double data);

//模拟下载，并通过进度条显示对应下载进度
void DownLoad(callback_t cd)
{
    double progress = 0;
    while(progress <= data)
    {
        cd(progress,data);
        usleep(10000);
        progress += speed;
    }
    printf("\n");
}


int main(){

    DownLoad(process);
    return 0;
}