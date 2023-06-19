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


// 좌표 구조체 정의
typedef struct {
    int x;
    int y;
} Pos;

// 리스트 구조체 정의
typedef struct {
    Pos* data;      // 데이터 배열
    int size;       // 현재 리스트의 크기
    int capacity;   // 리스트의 용량 (할당된 메모리 크기)
} List;

// 리스트 초기화
void initList(List* list);

// 리스트에 원소 추가
void appendList(List* list, int x, int y);

// 리스트에서 원소 가져오기
Pos getListElement(const List* list, int index);

void reverseList(List* list);

// 리스트 해제
void freeList(List* list);

#endif