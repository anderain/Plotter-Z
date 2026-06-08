#include <stdlib.h>
#include "vlist.h"

VlistNode* vlnNewNode(void *pData) {
    VlistNode* n = (VlistNode *)malloc(sizeof(VlistNode));
    n->pPrev = n->pNext = NULL;
    n->pData = pData;
    return n;
}

Vlist* vlNewList() {
    Vlist *l = (Vlist *)malloc(sizeof(Vlist));
    l->pHead = l->pTail = NULL;
    l->iSize = 0;
    return l;
}

Vlist* vlPushBack(Vlist* _self, void *pData) {
    VlistNode *newNode = vlnNewNode(pData);

    if (_self->pHead == NULL) {
        _self->pHead = _self->pTail = newNode;
    }
    else {
        VlistNode * pTail = _self->pTail;
        pTail->pNext = newNode;
        newNode->pPrev = pTail;
        _self->pTail = newNode;
    }

    _self->iSize++;

    return _self;
}

Vlist* vlPushFront(Vlist* _self, void *pData) {
    VlistNode *newNode = vlnNewNode(pData);

    if (_self->pHead == NULL) {
        _self->pHead = _self->pTail = newNode;
    }
    else {
        VlistNode * pHead = _self->pHead;
        pHead->pPrev = newNode;
        newNode->pNext = pHead;
        _self->pHead = newNode;
    }

    _self->iSize++;

    return _self;
}

void* vlPopFront(Vlist* _self) {
    VlistNode *pHead;
    void *pData;

    pHead = _self->pHead;

    if (pHead == NULL) return NULL;

    pData = pHead->pData;

    _self->pHead = pHead->pNext;
    if (_self->pHead) {
        _self->pHead->pPrev = NULL;
    }
    else {
        _self->pTail = NULL;
    }

    free(pHead);
    _self->iSize--;

    return pData;
}

void* vlPopBack(Vlist* _self) {
    VlistNode *pTail, *prevTail; 
    void * pData;

    if (_self->iSize <= 0) {
        return NULL;
    }

    pTail = _self->pTail;
    pData = pTail->pData;

    if (_self->iSize == 1) {
        _self->pHead = NULL;
        _self->pTail = NULL;
        free(pTail);
        _self->iSize = 0;
        return pData;
    }

    prevTail = pTail->pPrev;
    prevTail->pNext = NULL;
    _self->pTail = prevTail;
    free(pTail);
    _self->iSize--;

    return pData;
}

void vlnDestroy(VlistNode* vln, void (* releaseData)(void *)) {
    if (vln->pData != NULL && releaseData) {
        releaseData(vln->pData);
        vln->pData = NULL;
    }
    free(vln);
}

void vlDestroy(Vlist* _self, void (* releaseData)(void *)) {
    VlistNode *n1, *n2;

    n1 = _self->pHead;
    while (n1) {
        n2 = n1->pNext;
        vlnDestroy(n1, releaseData);
        n1 = n2;
    }

    free(_self);
}