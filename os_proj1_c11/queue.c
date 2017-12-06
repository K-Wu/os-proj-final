#ifndef QUEUE_HEADGUARD
#define QUEUE_HEADGUARD
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#define MAXNUM_CUSTOMER 800

pthread_mutex_t queue_lock;

int queue_n;
int queue_first;
int queue_last;

struct queue_Node{
    int customer_data_id;
};

struct queue_Node queue_data[MAXNUM_CUSTOMER];

void queue_init(){
    pthread_mutex_init(&queue_lock,NULL);
}



int queue_enqueue(int customer_data_id){//return ticket number
    pthread_mutex_lock(&queue_lock);
    queue_data[queue_last].customer_data_id=customer_data_id;
    queue_last++;
    pthread_mutex_unlock(&queue_lock);
    return (queue_last-1);
}

struct queue_Node* queue_peak_last(){
    return &queue_data[queue_last];
}

struct queue_Node* queue_peak_by_index(int index){
    if (index>=queue_last){
        fputs("Error: peak position exceeds last element in queue",stderr);
    }
    return &queue_data[index];
}

int queue_isEmpty(){
    return queue_first==queue_last;
}

struct queue_Node* queue_dequeue(){
    pthread_mutex_lock(&queue_lock);
    if (queue_isEmpty()){
        //fputs("Error: dequeue an empty queue",stderr);
        pthread_mutex_unlock(&queue_lock);
        return NULL;
    }
    queue_first++;
    pthread_mutex_unlock(&queue_lock);
    return &queue_data[queue_first-1];
}

//int main(){
//    queue_init();
//    queue_enqueue(3);
//    queue_enqueue(9);
//    queue_enqueue(11);
//    queue_enqueue(25);
//    queue_enqueue(27);
//    int b = queue_dequeue()->customer_data_id;
//    queue_enqueue(19);
//    queue_enqueue(55);
//    queue_enqueue(43);
//    queue_enqueue(522);
//    int c=queue_dequeue()->customer_data_id;
//    int d=queue_dequeue()->customer_data_id;
//    int e=queue_dequeue()->customer_data_id;
//    int f=queue_dequeue()->customer_data_id;
//    int g=queue_dequeue()->customer_data_id;
//    int h=queue_dequeue()->customer_data_id;
//    int i=queue_dequeue()->customer_data_id;
//    int j=queue_dequeue()->customer_data_id;
//    queue_enqueue(15);
//    int k=queue_dequeue()->customer_data_id;
//    if (queue_isEmpty()) k=222;
//    return 0;
//}

#endif