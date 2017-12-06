#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
char test_str[80]="-1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s -1s";

int main(void){
    int fd;
    if ((fd=open("/dev/scpd0",O_RDWR))<0)
        {printf("open error %d\n",fd);
         return -1;}
    while(1){
    printf("write once(test block)\n");
    usleep(500000);
    write(fd,test_str,79);
    }



    return 0;
}
