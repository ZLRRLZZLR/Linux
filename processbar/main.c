#include"process.h"
#include<stdio.h>
#include<unistd.h>

double data = 1024.0;
double speed = 1.0;

//模拟下载，并通过进度条显示对应下载进度
void DownLoad()
{
    double progress = 0;
    while(progress <= data)
    {
        process(progress,data);
        usleep(10000);
        progress += speed;
    }
    printf("\n");
}


int main(){

    DownLoad();
    return 0;
}