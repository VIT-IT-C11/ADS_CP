#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

struct  node
{
    int data;
    struct node* next;
};

struct queue{
    int size, capacity;
    struct node* front, *rear;
}q;


int enqueue(int data){
    struct node* temp=(struct node*)malloc(sizeof(struct node));
    temp->data=data;
    if(q.size>q.capacity){
        return 0;
    }
    if(q.rear==NULL){
        q.front=q.rear=temp;
        q.rear->next=NULL;
        q.size++;
        return 1;
    }
    q.rear->next=temp;
    q.rear=temp;
    q.rear->next=NULL;
    q.size++;
    return 1;
}

int dequeue(){
    
    struct node* temp;
    temp=q.front;
    q.front=q.front->next;
    int retdat = temp->data;
    free(temp);
    q.size--;
    return retdat;

    
}


void createQueue(int capacity){
    q.capacity=capacity;
    q.size=0;
    q.rear=NULL;
    q.front=NULL;
}
