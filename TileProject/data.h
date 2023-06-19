#pragma once

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 400


typedef struct {
    int F;
    int G;
    int Y;
    int X;
} PQNode;

int CompareTo(PQNode original, PQNode other);

typedef struct {
    int priority;
    PQNode data;
} Element;

typedef struct {
    Element heap[MAX_SIZE];
    int size;
} PriorityQueue;

PriorityQueue* createPriorityQueue();
void enqueue(PriorityQueue* pq, int priority, PQNode data);
PQNode dequeue(PriorityQueue* pq);


// ��ǥ ����ü ����
typedef struct {
    int x;
    int y;
} Pos;

// ����Ʈ ����ü ����
typedef struct {
    Pos* data;      // ������ �迭
    int size;       // ���� ����Ʈ�� ũ��
    int capacity;   // ����Ʈ�� �뷮 (�Ҵ�� �޸� ũ��)
} List;

// ����Ʈ �ʱ�ȭ
void initList(List* list);

// ����Ʈ�� ���� �߰�
void appendList(List* list, int x, int y);

// ����Ʈ���� ���� ��������
Pos getListElement(const List* list, int index);

void reverseList(List* list);

// ����Ʈ ����
void freeList(List* list);

#endif