#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue.c"
#include <time.h>
#include <semaphore.h>
#define PARTITION_THRESH 1000

//#define WORKER_MAXNUM 20
int WORKER_MAXNUM;
int DATA_LENGTH=1000000;
typedef double dtype;
dtype data[DATA_MAXLENGTH];
int worker_num;

//pthread_t worker[WORKER_NUM];
//sem_t available_worker_num;
pthread_mutex_t lock;
pthread_cond_t scheduler_cond;


void generate_random_data(){
    int i;
    srand((int)time(0));
    for (int i=0;i<DATA_LENGTH;i++){
        data[i]=(100.0*rand())/(RAND_MAX+1.0);
    }
}
void write_data_to_file(const char* filename){
    FILE* fd=fopen(filename,"w");
    if (fd==NULL){
        fputs ("File error",stderr);

    }

        fwrite(&data,sizeof(data[0]),DATA_LENGTH,fd);
    fclose(fd);
}
int comp(const void *a, const void *b){
    const dtype * pa = (const dtype *)a;
    const dtype * pb = (const dtype *)b;
    return (*pa<*pb)?-1:(*pa >*pb);
}
void swap(int a,int b){
    dtype tmp=data[a];
    data[a]=data[b];
    data[b]=tmp;
}
void* partition(void* param){
    struct queue_Node* args=(struct queue_Node *) param;
    int lo=args->beg;
    int hi=args->end;
    if (hi-lo<=PARTITION_THRESH)
    {
        qsort(&data[lo],hi-lo+1,sizeof(dtype),comp);
        dbg_printf("qsort locked partition %d %d worker num: %d\n",lo,hi,worker_num);
        pthread_mutex_lock(&lock);
        dbg_printf("qsort partition entering lock %d %d worker num: %d\n",lo,hi,worker_num);
    }
    else {
        int i = lo;
        int j = hi + 1;
        dtype pivot = data[lo];
        while (1) {
            while (data[++i] < pivot)
                if (i == hi) break;
            while (data[--j] > pivot)
                if (j == lo) break;
            if (i >= j) break;
            swap(i, j);
        }
        swap(lo, j);

        dbg_printf("locked partition %d %d worker num: %d\n",lo,hi,worker_num);
        pthread_mutex_lock(&lock);
        dbg_printf("partition entering lock %d %d worker num: %d\n",lo,hi,worker_num);
        queue_enqueue(lo,j-1);
        queue_enqueue(j+1,hi);


    }
     //现在划分元在j
    //sem_post(&available_worker_num);
    worker_num--;
    pthread_cond_signal(&scheduler_cond);
    pthread_mutex_unlock(&lock);
    dbg_printf("partition leaving lock %d %d worker num: %d\n",lo,hi,worker_num);
    pthread_exit(NULL);
    }

void load_data_from_file(const char* filename){
    FILE * fd=fopen(filename,"r");
    if (fd==NULL){
        fputs("File error",stderr);
    }
    dtype tmp;
    int index=0;
    while (index<DATA_LENGTH&&fread(&data[index],sizeof(data[0]),1,fd)){//fread pairs with fwrite
        index++;
    }
    fclose(fd);

}
void init(){

    int err_no=0;
    err_no|=queue_init();
    //err_no|=sem_init(&available_worker_num,0,WORKER_MAXNUM);
    err_no|=pthread_mutex_init(&lock,NULL);
    err_no|=pthread_cond_init(&scheduler_cond,NULL);
    queue_enqueue(0,DATA_LENGTH-1);
    if (err_no!=0){
        fputs("At least one step in init() fails, terminating this program",stderr);
        exit(-1);
    }
}

int data_isSorted(){
    for(int i=0;i<DATA_LENGTH-1;i++){
        if(data[i]>data[i+1])
            return 0;
            //printf("Error: not sorted index: %d\n",i);
    }
    return 1;
}

void* scheduler(){
    while(1)
    {
        //sem_wait(&available_worker_num);

        pthread_mutex_lock(&lock);
        if(queue_isEmpty())
        {

            printf("  Queue empty\n");
            //sem_post(&available_worker_num);
            if(worker_num==0)
            {
                pthread_mutex_unlock(&lock);
                break;

            }//done
            else
            {//线程未空，队列空
                //pthread_mutex_unlock(&lock);

                pthread_cond_wait(&scheduler_cond,&lock);
                pthread_mutex_unlock(&lock);
            }
        }
        else if (worker_num<WORKER_MAXNUM){

            struct queue_Node* q_n = queue_dequeue();
            pthread_t tht;
            worker_num++;
            printf("queue_first: %d queue_last: %d worker_num: %d\n",queue_first,queue_last,worker_num);
            while(pthread_create(&tht,NULL,partition,(void *)q_n))
                printf("Error: thread creation\n");
            pthread_mutex_unlock(&lock);
            pthread_detach(tht);
//            int err_no = pthread_create(&tht,NULL,partition,(void *)q_n);
//            if (err_no){
//                printf("  Error: error when creating partition thread\n");
//                pthread_mutex_lock(&lock);
//                worker_num--;
//                pthread_mutex_unlock(&lock);
//                sem_post(&available_worker_num);
//            }
        }
        else{//队列非空，线程满
            printf("  Worker full\n");
            pthread_cond_wait(&scheduler_cond,&lock);
            pthread_mutex_unlock(&lock);
        }

    }

}

__time_t ClockGetTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (__time_t)ts.tv_sec * 1000000LL + (__time_t)ts.tv_nsec / 1000LL;
}

int main(const int argc, const char * argv[]) {
//    generate_random_data();
//    write_data_to_file("data3.dat");
//    generate_random_data();
//    load_data_from_file("data1.dat");
//    write_data_to_file("data2.dat");

    if (argc != 4){
        printf("invalid arguments\nUsage: ./main RANDOM DATA_LENGTH/FILENAME THREADS_NUM\nRANDOM: 1 would be random generation, 0 will be reading data from file\nDATA_LENGTH: no more than 1e7\nTHREADS_NUM: 1 will disable multithread, reducing to a simple qsort calling\n");
        exit(-1);
    }
    sscanf(argv[3],"%d",&WORKER_MAXNUM);
    int isRandom;
    sscanf(argv[1],"%d",&isRandom);
    if (isRandom){
        sscanf(argv[2],"%d",&DATA_LENGTH);
        generate_random_data();
    }
    else {
        load_data_from_file(argv[2]);
    }
    __time_t t_start=ClockGetTime();
    if (WORKER_MAXNUM==1){
        qsort(data,DATA_LENGTH,sizeof(dtype),comp);
    }
    else{
    init();
    pthread_t s_th;
    if (pthread_create(&s_th,NULL,scheduler,NULL))
    {
        fputs("error when creating scheduler",stderr);
    }
    pthread_join(s_th,NULL);
    }
    if (data_isSorted()){
        printf("Now the data is sorted\n");
    }
    else{
        fputs("Error: data is not sorted\n",stderr);
    }
    __time_t t_end=ClockGetTime();
    __time_t t_elapse=(t_end-t_start);
    printf("time elapsed: %ld micro sec",t_elapse);
    return 0;
}