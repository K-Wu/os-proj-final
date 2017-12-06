#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#define COUNTER_MAXNUM 30
#include "queue.c"
int COUNTER_NUM;
#define VERBOSEx
#ifdef VERBOSE
# define verbose_printf(...) printf(__VA_ARGS__)
#else
# define verbose_printf(...)
#endif
sem_t waiting_customers;
sem_t check_finish;
struct customer_args{
    pthread_t customer_t;
    int data_id;//和其在customer_data中的序号一致
    int input_id;
    long counter_id;
    int enter_time;
    int wait_time;
    pthread_t pt;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};
struct counter{
    pthread_t pt;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};
struct counter counter_data[COUNTER_MAXNUM];
struct customer_args customer_data[MAXNUM_CUSTOMER];
int customer_num;
pthread_mutex_t finished_customer_num_lock;
pthread_cond_t finished_customer_num_cond;
int finished_customer_num;
void Sleep(unsigned int time){
    usleep(time*100);
}

void* counter(void* arg){
    long int counter_id=(long)arg;
    int time=0;
    verbose_printf("counter id: %d starting his job\n",counter_id);
    while(1){

        //printf("counter_id %d waiting\n",counter_id);
        sem_wait(&waiting_customers);//信号量。特别地，当不存在等待顾客时陷入阻塞
        struct queue_Node* c_args=queue_dequeue();//从队列中获得顾客信息
        if (c_args==NULL)
        {
            verbose_printf("counter id %ld unexpectedly not blocked by semaphore\n",counter_id);
            continue;
        }
        if (customer_data[c_args->customer_data_id].counter_id!=-1){
            printf("panic: Counter id %ld found Customer id %d is already called by counter id %ld when trying to call it\n",
            counter_id,c_args->customer_data_id,customer_data[c_args->customer_data_id].counter_id);
            continue;
        }
        pthread_mutex_lock(&customer_data[c_args->customer_data_id].lock);//给顾客上锁，防止不同柜台同时叫一个顾客（号）

        int customer_data_id = c_args->customer_data_id;
        customer_data[c_args->customer_data_id].counter_id=counter_id;//修改信息：顾客由本柜台服务，同时顾客也由此信息退出会陷入阻塞的循环
        pthread_mutex_lock(&counter_data[counter_id].lock);//给自己上锁
        verbose_printf("   counter id :%ld lock one customer data_id %d\n",counter_id,c_args->customer_data_id);
        pthread_cond_signal(&customer_data[c_args->customer_data_id].cond);//告诉顾客他被叫到了，该条件变量由顾客锁保护
        pthread_mutex_unlock(&customer_data[c_args->customer_data_id].lock);//解开顾客锁

        verbose_printf("   counter id :%ld unlock one customer data_id %d\n",counter_id,c_args->customer_data_id);
        pthread_cond_wait(&counter_data[counter_id].cond,&counter_data[counter_id].lock);//等待顾客告知柜台服务完毕
        pthread_mutex_unlock(&counter_data[counter_id].lock);//解开柜台锁
        //完成了接待这名顾客
        time+=customer_data[c_args->customer_data_id].wait_time;//统计时间
        printf("customer id %d entering at time %d leaving at time %d, served by counter id %ld\n",c_args->customer_data_id,(time-customer_data[c_args->customer_data_id].wait_time),time,counter_id);
        pthread_mutex_lock(&finished_customer_num_lock);
        finished_customer_num++;
        pthread_cond_signal(&finished_customer_num_cond);
        pthread_mutex_unlock(&finished_customer_num_lock);
    }
}

void* customer(void* arg){

    struct customer_args* args=(struct customer_args *) arg;

    Sleep(args->enter_time);//等待时间
    verbose_printf("Time %d: customer data_id: %d entering system\n",args->enter_time,args->data_id);

    pthread_cond_init(&customer_data[args->data_id].cond,NULL);//初始化条件变量和锁
    pthread_mutex_init(&customer_data[args->data_id].lock,NULL);
    pthread_cond_t* customer_cond_ptr = &customer_data[args->data_id].cond;
    pthread_mutex_t* customer_mutex_ptr = &customer_data[args->data_id].lock;
    pthread_mutex_lock(customer_mutex_ptr);//防止取到票（入队列）以后直接被柜台调走
    int ticket_no = queue_enqueue(args->data_id);//入队列，得到排队编号
    sem_post(&waiting_customers);//信号量。特别地，当有柜台因无顾客而信号量阻塞时，唤醒正在等待顾客的柜台
    while (customer_data[args->data_id].counter_id==-1){//还没有柜员叫到
        verbose_printf("customer data_id %d waiting\n",args->data_id);//测试是否正确实现阻塞
        pthread_cond_wait(customer_cond_ptr,customer_mutex_ptr);//阻塞释放锁
    }

    int counter_id=customer_data[args->data_id].counter_id;//柜员叫到
    verbose_printf("customer data_id: %d is served by counter_id %d\n",args->data_id,counter_id);
    pthread_mutex_lock(&counter_data[counter_id].lock);//给柜员上锁
    verbose_printf("  customer data_id: %d is locked\n",args->data_id);
    Sleep(customer_data[args->data_id].wait_time);//等待服务时间结束
    pthread_cond_signal(&counter_data[counter_id].cond);//告诉柜员服务完毕，该条件变量由柜员锁保护
    verbose_printf("  customer data_id: %d finished, now unlocking\n",args->data_id);
    pthread_mutex_unlock(&counter_data[counter_id].lock);//释放柜员的锁
    verbose_printf("customer data_id: %d done\n",args->data_id);



    pthread_mutex_unlock(&(*customer_mutex_ptr));//退出前，释放自己的锁
}

void read_customer_from_file(const char* filename){
    FILE* fd=fopen(filename,"r");
    int id,enter_time,wait_time;
    while(fscanf(fd,"%d %d %d\n",&id,&enter_time,&wait_time)!=EOF)
    {
        customer_data[customer_num].data_id=customer_num;
        customer_data[customer_num].input_id=id;
        customer_data[customer_num].enter_time=enter_time;
        customer_data[customer_num].wait_time=wait_time;
        customer_data[customer_num].counter_id=-1;
        customer_num++;
    }
}

void init(){
    queue_init();
    int err_code=0;
    err_code|=sem_init(&waiting_customers,0,0);
    err_code|=pthread_cond_init(&finished_customer_num_cond,NULL);
    err_code|=pthread_mutex_init(&finished_customer_num_lock,NULL);
    for (long i=0;i<COUNTER_NUM;i++)
    {
        err_code|=pthread_cond_init(&counter_data[i].cond,NULL);
        err_code|=pthread_mutex_init(&counter_data[i].lock,NULL);
        err_code|=pthread_create(&counter_data[i].pt,NULL,counter,(void *)i);
    }

    for (int i=0;i<customer_num;i++)
    {
        err_code|=pthread_cond_init(&customer_data[i].cond,NULL);
        err_code|=pthread_mutex_init(&customer_data[i].lock,NULL);
        err_code|=pthread_create(&customer_data[i].pt,NULL,customer,(void *)&customer_data[i]);
    }
    if (err_code!=0)
    {
        fputs("At least one step in initialization does not succeed, exit\n",stderr);
        exit(-1);
    }
//    customer_data[0].enter_time=5;
//    customer_data[0].data_id=3;
//    customer_data[0].wait_time=8;
//
//    customer_data[1].enter_time=3;
//    customer_data[1].data_id=1;
//    customer_data[1].wait_time=14;
//    pthread_t aa[2];
//    pthread_create(&aa[0],NULL,customer,(void *) &customer_data[0]);
//    pthread_create(&aa[1],NULL,customer,(void *) &customer_data[1]);
}
int main(const int argc, const char * argv[]) {
    if (argc !=4){
        printf("invalid arguments\nUsage: ./main COUNTER_NUM SIMULATION_TIME FILENAME\nCOUNTER_NUM: no more than 30\nSIMULATION_TIME 0 to end automatically\n");
        exit(-1);
    }
    sscanf(argv[1],"%d",&COUNTER_NUM);
    int SLEEP_TIME;
    sscanf(argv[2],"%d",&SLEEP_TIME);
    //printf("Hello, World!\n");
    read_customer_from_file(argv[3]);
    init();
    if (SLEEP_TIME!=0)
        Sleep(SLEEP_TIME);
    else{

        pthread_mutex_lock(&finished_customer_num_lock);
        while(finished_customer_num!=customer_num){
            pthread_cond_wait(&finished_customer_num_cond,&finished_customer_num_lock);

        }
        pthread_mutex_unlock(&finished_customer_num_lock);
    }
    //pthread_join(counter_data[0].pt,NULL);

    return 0;
}