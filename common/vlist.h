#ifndef _V_LIST_H_
#define _V_LIST_H_

typedef struct tagVlistNode {
    struct tagVlistNode *pPrev, *pNext;
    void *pData;
} VlistNode;

typedef struct {
    VlistNode *pHead, *pTail;
    int iSize;
} Vlist;

Vlist*      vlNewList       ();
Vlist*      vlPushBack      (Vlist* _self, void *pData);
void*       vlPopFront      (Vlist* _self);
void*       vlPopBack       (Vlist* _self);
void        vlDestroy       (Vlist* _self, void (* releaseData)(void *));

#define vlPeek(_self)       ((_self)->pTail->pData)

typedef Vlist VQueue;
#define vqNewQueue                  vlNewList
#define vqPush                      vlPushBack
#define vqPop                       vlPopFront
#define vqIsEmpty(q)                ((q)->iSize <= 0)
#define vqDestroy(q, releaseData)   vlDestroy(q, releaseData)

#endif