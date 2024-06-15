#include "matte_pool.h"
#include "matte_array.h"
#include "matte_au32.h"
#include <stdlib.h>
#include <string.h>
#ifdef MATTE_DEBUG
    #include <assert.h>
#endif
#include "matte.h"
#include <stdio.h>


#define POOL_PAGE_SIZE 1024
#define POOL_PAGE_SIZE_DEAD 4096

//#define POOL_PAGE_SIZE 16
//#define POOL_PAGE_SIZE_DEAD 16


typedef struct  {
    uint32_t fillCount;
    uint32_t useCount; // for cleanup
    void * data;
} mattePoolPage_t;

struct mattePool_t {
    matteArray_t * dead;
    matteArray_t * pages;
    uint32_t size;
    uint32_t sizeofType;
    void (*cleanup)(void *);
};




mattePool_t * matte_pool_create(uint32_t sizeofType, void (*cleanup)(void *)) {
    mattePool_t * out = matte_allocate(sizeof(mattePool_t));
    out->pages = matte_array_create(sizeof(mattePoolPage_t));
    out->dead  = matte_array_create(sizeof(mattePoolPage_t));
    out->size = 0;
    out->sizeofType = sizeofType;
    out->cleanup = cleanup;
    return out;
}

// Destroys a node pool
void matte_pool_destroy(mattePool_t * m) {
    uint32_t i;
    if (m->cleanup) {
        for(i = 0; i < m->pages->size; ++i) {
            mattePoolPage_t * page = &matte_array_at(m->pages, mattePoolPage_t, i);
            uint32_t n;
            for(n = 0; n < page->useCount; ++n) {
                m->cleanup(
                    ((uint8_t*)page->data) + m->sizeofType * n
                );
            }
        }
    }


    uint32_t len = m->pages->size;
    for(i = 0; i < len; ++i) {
        matte_deallocate(
            matte_array_at(m->pages, mattePoolPage_t, i).data
        );
    }

    len = m->dead->size;
    for(i = 0; i < len; ++i) {
        matte_deallocate(
            matte_array_at(m->dead, mattePoolPage_t, i).data
        );
    }


    matte_array_destroy(m->pages);
    matte_array_destroy(m->dead);
    matte_deallocate(m);
}

static uint32_t next_dead(mattePool_t * m) {
    mattePoolPage_t * top = &matte_array_at(m->dead, mattePoolPage_t, m->dead->size-1);
    uint32_t next = ((uint32_t*)top->data)[top->fillCount-1];

    top->fillCount -= 1;
    if (top->fillCount == 0) {
        matte_deallocate(top->data);
        top->data = NULL;
        m->dead->size--;
    }   
    return next;
}

static int has_next_dead(mattePool_t * m) {
    return m->dead->size != 0 && matte_array_at(m->dead, mattePoolPage_t, m->dead->size-1).fillCount != 0;
}

// Creates a new node from the pool
uint32_t matte_pool_add(mattePool_t * m) {
    uint32_t id = 0xffffffff;
    if (has_next_dead(m)) {
        do {
            id = next_dead(m);
            if (id/POOL_PAGE_SIZE < m->pages->size) {
                break;
            }

            // rare case, empty deads
            if (m->dead->size == 0) {
                id = m->size++;
                break;
            } 
        } while(has_next_dead(m));
    } else {
        id = m->size++;
    }
        
    uint32_t page = id / POOL_PAGE_SIZE;
    if (page >= m->pages->size) {
        mattePoolPage_t page = {};
        page.data = NULL; 
        page.fillCount = 0;
        matte_array_push(m->pages, page);
    }
    mattePoolPage_t * pageV = ((mattePoolPage_t*)m->pages->data)+(id/POOL_PAGE_SIZE);

    if (pageV->data == NULL) {
        pageV->data = matte_allocate(m->sizeofType * POOL_PAGE_SIZE);
    }
    pageV->fillCount++;
    if (pageV->fillCount > pageV->useCount)
        pageV->useCount = pageV->fillCount;
    return id;
}



// destroys a node from the pool;
void matte_pool_recycle(mattePool_t * m, uint32_t id) {

    mattePoolPage_t * page = ((mattePoolPage_t*)m->pages->data)+(id/POOL_PAGE_SIZE);
    
    if (m->dead->size == 0 || (matte_array_at(m->dead, mattePoolPage_t, m->dead->size-1).fillCount >= POOL_PAGE_SIZE_DEAD)) {
        mattePoolPage_t dead = {};
        dead.data = matte_allocate(sizeof(uint32_t) * POOL_PAGE_SIZE_DEAD);
        dead.fillCount = 0;
        matte_array_push(m->dead, dead);
    }

    mattePoolPage_t * dead = &matte_array_at(m->dead, mattePoolPage_t, m->dead->size-1);
    ((uint32_t*)dead->data)[dead->fillCount] = id;
    dead->fillCount++;
    page->fillCount--;
    if (page->fillCount == 0) {
        if (m->cleanup) {
            uint32_t i = 0;
            for(i = 0; i < page->useCount; ++i) {
                m->cleanup(
                    ((uint8_t*)page->data) + m->sizeofType * i 
                );
            }
        }


        matte_deallocate(page->data);
        page->data = NULL;
        page->useCount = 0;
        if (id/POOL_PAGE_SIZE >= m->pages->size-1) {
        
            m->pages->size--;
            m->size = ((m->pages->size) * POOL_PAGE_SIZE);
        }
    }
}

void * matte_pool_fetch_raw(mattePool_t * m, uint32_t id) {
    mattePoolPage_t * pageV = ((mattePoolPage_t*)m->pages->data)+(id/POOL_PAGE_SIZE);
    if (pageV->data == NULL) return NULL;
    return (void*)(((uint8_t*)pageV->data) + (id%POOL_PAGE_SIZE) * m->sizeofType);
}

void matte_pool_report(mattePool_t * m) {
    double averageFillRate = 0;
    double B = 0;
    uint32_t i;
    uint32_t len = m->pages->size;
    for(i = 0; i < len; ++i) {
        mattePoolPage_t * pool = &matte_array_at(m->pages, mattePoolPage_t, i);
        double frac = pool->fillCount / (float) POOL_PAGE_SIZE;
        averageFillRate += frac;
        printf("Pool %5d:  %3d / %d ",
            i,
            (int) pool->fillCount,
            (int) POOL_PAGE_SIZE
        );
        
        if (pool->fillCount) {
            printf("[");
            uint32_t n;
            for(n = 0; n < 20*frac; ++n) {
                printf("=");
            }
            for(; n < 20; ++n) {
                printf(" ");
            }
            printf("]\n");
        } else {
            printf("\n");
        }
        B += ((pool->data) ? POOL_PAGE_SIZE : 0) * m->sizeofType;
    }
    
    printf("Average fill rate: %d%%\n", (int) (100*(averageFillRate / len)));
    printf("Size             : From alive %d KB\n", (int) (B/1024));
    printf("                 : From dead  %d KB\n", (int) (m->dead->allocSize * sizeof(uint32_t))/1024);
}


