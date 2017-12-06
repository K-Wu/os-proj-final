//
// Created by tonywukun on 17-11-12.
//
#include<stdlib.h>
#include<memory.h>
struct queue_Node{
    queue_Node* next;
    int beg;
    int end;
};
queue_Node* queue_first;
queue_Node* queue_last;
pthread_mutex_t queue_lock;
int queue_n;
void queue_init(){
    queue_first=NULL;
    queue_last=NULL;
    queue_n=0;
    pthread_mutex_init(&queue_lock);
}

bool queue_isEmpty(){
    return queue_first==NULL;
}
int queue_size(){
    return queue_n;
}

struct queue_Node* create_queue_node(){
    void* a=malloc(sizeof(struct queue_Node));
    return (struct queue_Node*) a;
}

void free_queue_Node(cqueue_Node* ptr){
    free(ptr);
}

void queue_enqueue(int beg, int end){
    pthread_mutex_lock(queue_lock);
    queue_Node* oldlast=queue_last;
    queue_last=create_queue_node();
    queue_last->beg=beg;
    queue_last->end=end;
    queue_last->next=NULL;
    if (queue_isEmpty()) queue_first=queue_last;
    else oldlast->next=queue_last;
    queue_n++;
    pthread_mutex_unlock(queue_lock);
}


struct queue_Node queue_dequeue(){
    pthread_mutex_lock(queue_lock);
    if (queue_isEmpty()){
        pthread_mutex_unlock(queue_lock);
        return NULL;
    }
    queue_Node result;
    result.beg=queue_first->beg;
    result.end=queue_first->end;
    queue_Node* tmp=queue_first->next;
    struct queue_Node* to_free=queue_first;
    queue_first=tmp;
    queue_n--;

    if (queue_isEmpty()) queue_last=NULL;
    pthread_mutex_unlock(queue_lock);
    free_queue_Node(to_free);
    return result;
}

//int main(){
//    queue_init();
//    queue_enqueue(3,5);
//    queue_enqueue(4,9);
//    queue_enqueue(8,11);
//    queue_enqueue(25,3);
//    queue_enqueue(27,9);
//    int a = queue_peek_next_task_beg();
//    int b = queue_dequeue();
//    queue_enqueue(19,23);
//    queue_enqueue(21,55);
//    queue_enqueue(7,43);
//    queue_enqueue(8,522);
//    int c=queue_dequeue();
//    int d=queue_dequeue();
//    int e=queue_dequeue();
//    int f=queue_dequeue();
//    int g=queue_dequeue();
//    int h=queue_dequeue();
//    int i=queue_dequeue();
//    int j=queue_dequeue();
//    queue_enqueue(15,7);
//    int k=queue_dequeue();
//    return 0;
//}
