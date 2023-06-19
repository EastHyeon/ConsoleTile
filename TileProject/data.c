#include "data.h"

PriorityQueue* createPriorityQueue() {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->size = 0;
    return pq;
}

int CompareTo(PQNode original, PQNode other) {
    if (original.F == other.F)
        return 0;
    return original.F < other.F ? 1 : -1;
}

void enqueue(PriorityQueue* pq, int priority, PQNode data) {
    if (pq->size >= MAX_SIZE) {
        return;
    }

    int i = pq->size;

    Element elem = { priority, data };

    while (i > 0 && elem.priority < pq->heap[(i - 1) / 2].priority) {
        pq->heap[i] = pq->heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    pq->heap[i] = elem;
    pq->size++;
}

PQNode dequeue(PriorityQueue* pq) {
    if (pq->size <= 0) {
        Element elem = { -1, -1 };
        return elem.data;
    }

    Element result = pq->heap[0];
    Element lastElem = pq->heap[pq->size - 1];
    pq->size--;

    int parent = 0;
    int child = 1;

    while (child < pq->size) {
        if (child < pq->size - 1 && pq->heap[child].priority > pq->heap[child + 1].priority)
            child++;

        if (lastElem.priority <= pq->heap[child].priority)
            break;

        pq->heap[parent] = pq->heap[child];
        parent = child;
        child = 2 * parent + 1;
    }

    pq->heap[parent] = lastElem;

    return result.data;
}

// 리스트 초기화
void initList(List* list) {
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

// 리스트에 원소 추가
void appendList(List* list, int x, int y) {
    if (list->size >= list->capacity) {
        // 현재 용량을 두 배로 늘린다
        int newCapacity = (list->capacity == 0) ? 1 : list->capacity * 2;
        Pos* newData = (Pos*)realloc(list->data, newCapacity * sizeof(Pos));
        if (newData == NULL) {
            fprintf(stderr, "메모리 할당 오류\n");
            return;
        }
        list->data = newData;
        list->capacity = newCapacity;
    }

    list->data[list->size].x = x;
    list->data[list->size].y = y;
    list->size++;
}

// 리스트에서 원소 가져오기
Pos getListElement(const List* list, int index) {
    Pos pos;
    if (index >= 0 && index < list->size) {
        pos = list->data[index];
    }
    else {
        fprintf(stderr, "잘못된 인덱스\n");
        pos.x = -1;  // 또는 다른 오류 처리 방식을 선택할 수 있음
        pos.y = -1;
    }
    return pos;
}

void reverseList(List* list) {
    for (int i = 0; i < list->size / 2; i++) {
        // 요소를 역순으로 출력
        Pos temp = list->data[i];
        list->data[i] = list->data[list->size - 1 - i];
        list->data[list->size - 1 - i] = temp;
    }
}

// 리스트 해제
void freeList(List* list) {
    free(list->data);
    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}