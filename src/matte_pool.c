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


#define POOL_PAGE_SIZE 256


typedef struct mattePoolPage_t mattePoolPage_t;

struct mattePoolPage_t {
    // used for cleanup; marks how many cells are utilized within this page
    uint32_t useCount; 
    uint32_t id;
    void * data;
    // marked as filled when each taken[] index is full.
    uint8_t filled[POOL_PAGE_SIZE / 64];
    uint64_t inUse[POOL_PAGE_SIZE / 64];
    
    // when unfilled, these point to the next and prev unfilled.
    mattePoolPage_t * next;
    mattePoolPage_t * prev;
};



struct mattePool_t {
    // first page is always the least filled
    mattePoolPage_t * unfilled;
    
    matteArray_t * pages;
    uint32_t size;
    uint32_t sizeofType;
    void (*cleanup)(void *);
};




mattePool_t * matte_pool_create(uint32_t sizeofType, void (*cleanup)(void *)) {
    mattePool_t * out = matte_allocate(sizeof(mattePool_t));
    out->unfilled = NULL;
    out->pages  = matte_array_create(sizeof(mattePoolPage_t));
    out->size = 0;
    out->sizeofType = sizeofType;
    out->cleanup = cleanup;
    return out;
}

static int matte_pool_check_fill(mattePoolPage_t * page, int total) {
    int n;
    int id = 0;
    for(n = 0; n < POOL_PAGE_SIZE/64; ++n) {
        int iii;
        for(iii = 0; iii < 64; ++iii) {
            if ((page->inUse[n] & (((uint64_t)1) << iii)) == 0)
                continue;
            id += 1;
        }
    }
    
    
    #ifdef MATTE_DEBUG__STORE
        assert(total == id);
    #endif
}

static void matte_printf_unfilled_chain(mattePool_t * m) {
    mattePoolPage_t * page = m->unfilled;
    printf("[");
    while(page) {
        printf("%d->", page->id);
        page = page->next;
    }
    printf("]\n");
}

// Destroys a node pool
void matte_pool_destroy(mattePool_t * m) {
    uint32_t i;
    if (m->cleanup) {
        for(i = 0; i < m->pages->size; ++i) {
            mattePoolPage_t * page = &matte_array_at(m->pages, mattePoolPage_t, i);
            if (page->data == NULL) continue;
            uint32_t n;
            uint32_t len = POOL_PAGE_SIZE/64;
            for(n = 0; n < len; ++n) {
                int iii;
                for(iii = 0; iii < 64; ++iii) {
                    if ((page->inUse[n] & (((uint64_t)1) << iii)) == 0)
                        continue;
                    m->cleanup(
                        ((uint8_t*)page->data) + m->sizeofType * (n * 64 + iii)
                    );
                }
            }
        }
    }


    uint32_t len = m->pages->size;
    for(i = 0; i < len; ++i) {
        matte_deallocate(
            matte_array_at(m->pages, mattePoolPage_t, i).data
        );
    }

    matte_array_destroy(m->pages);
    matte_deallocate(m);
}

void matte_page_report(mattePoolPage_t * page) {
    int i;
    int len = POOL_PAGE_SIZE/64;
    int first = -1;
    for(i = 0; i < len; ++i) {
        printf("\n%d - \n", i);

        int n;
        for(n = 0; n < 4; ++n) {
            int m;
            for(m = 0; m < 16; ++m) {
                int which = n * 16 + m;
                int taken = (page->inUse[i] & (((uint64_t)1) << which)) != 0;
                printf("%d", taken);
                
                if (first == -1 && taken == 0)
                    first = i * 64 + which;
                
                if (m > 0 && m%7 == 0)
                  printf("-");

            }
            printf("\n");
        }
    }    
    
    printf("\nFirst empty: %d\n", first);
}

static void matte_pool_unlink(mattePool_t * m, mattePoolPage_t * page) {
    mattePoolPage_t * pageNext = page->next;

    if (page->next) {
        page->next->prev = page->prev;
        page->next = NULL;
    }
    
    if (page->prev) {
        page->prev->next = pageNext;
        page->prev = NULL;
    }
}

// Creates a new node from the pool
uint32_t matte_pool_add(mattePool_t * m) {
    uint32_t id = 0xffffffff;
    if (m->unfilled) {
        mattePoolPage_t * page = m->unfilled;
        int i;
        int len = POOL_PAGE_SIZE/64;
        for(i = 0; i < len; ++i) {
            if (page->filled[i]) continue;
            int n;
            for(n = 0; n < 64; ++n) {
                if ((page->inUse[i] & (((uint64_t)1) << n)) == 0) {
                    id = page->id * POOL_PAGE_SIZE + i * 64 + n;
                    if (id >= m->size)
                        m->size = id+1;
                    goto L_FOUND;
                }
            }
        }
      L_FOUND:;
        #ifdef MATTE_DEBUG__STORE
            assert(id != 0xffffffff);
        #endif
        
    } else {
        id = m->size++;

    }

        
    uint32_t page = id / POOL_PAGE_SIZE;
    if (page >= m->pages->size) {
        mattePoolPage_t page = {}; // zeroes
        page.id = m->pages->size;
        matte_array_push(m->pages, page);
    }

    mattePoolPage_t * pageV = ((mattePoolPage_t*)m->pages->data)+(id/POOL_PAGE_SIZE);

    if (pageV->data == NULL) {
        pageV->data = matte_allocate(m->sizeofType * POOL_PAGE_SIZE);
    }
    uint32_t smallID = id%POOL_PAGE_SIZE;
    int smallIDslot = smallID/64;
    #ifdef MATTE_DEBUG__STORE
        assert((pageV->inUse[smallIDslot] & (((uint64_t)1) << smallID%64)) == 0); 
    #endif
    pageV->inUse[smallIDslot] |= (((uint64_t)1) << smallID%64); 
    if (pageV->inUse[smallIDslot] == (0xffffffffffffffffull)) {
        pageV->filled[smallIDslot] = 1;
    }
    
    
    #ifdef MATTE_DEBUG__STORE
        assert(pageV->useCount != 256);
    #endif
    pageV->useCount++;


    // remove from unfilled if filled.
    if (pageV->useCount == POOL_PAGE_SIZE) {
        if (m->unfilled == pageV)
            m->unfilled = pageV->next;
        matte_pool_unlink(m, pageV);
        #ifdef MATTE_DEBUG__STORE
            assert(pageV->next == NULL && pageV->prev == NULL);
        #endif  
    }


    return id;
}





// destroys a node from the pool;
void matte_pool_recycle(mattePool_t * m, uint32_t id) {

    mattePoolPage_t * pageV = ((mattePoolPage_t*)m->pages->data)+(id/POOL_PAGE_SIZE);
    uint32_t smallID = id%POOL_PAGE_SIZE;
    int smallIDslot = smallID/64;
    #ifdef MATTE_DEBUG__STORE
        assert((pageV->inUse[smallIDslot] & (((uint64_t)1) << smallID%64)) != 0); 
    #endif

    //matte_pool_check_fill(pageV, pageV->useCount);
    pageV->inUse[smallIDslot] &= ~(((uint64_t)1) << smallID%64);
    if (pageV->inUse[smallIDslot] != (0xffffffffffffffffull)) {
        pageV->filled[smallIDslot] = 0;
    }

    #ifdef MATTE_DEBUG__STORE
        assert(id == (pageV->id * POOL_PAGE_SIZE + smallIDslot * 64 + smallID%64));
    #endif

    pageV->useCount--;
    //matte_pool_check_fill(pageV, pageV->useCount);


    if (pageV->useCount == 0) {
        if (m->cleanup) {
            uint32_t n;
            for(n = 0; n < POOL_PAGE_SIZE; ++n) {
                m->cleanup(
                    ((uint8_t*)pageV->data) + m->sizeofType * n
                );
            }
        }
    
        matte_deallocate(pageV->data);
        pageV->data = NULL;
    }
    
    if (m->unfilled) {
        // already good!
        if (m->unfilled == pageV) return;

        if (pageV->useCount < m->unfilled->useCount) {
            matte_pool_unlink(m, pageV);
            m->unfilled->prev = pageV;
            pageV->next = m->unfilled;
            m->unfilled = pageV;
            pageV->prev = NULL;

        } else {
            // already in unfilled, ignore.
            if (pageV->next || pageV->prev) return;
            
            
            // dubious. take care of this for better consistency
            pageV->next = m->unfilled->next;
            if (pageV->next) {
                pageV->next->prev = pageV;
            }
            pageV->prev = m->unfilled;
            m->unfilled->next = pageV;

        }
    } else {
        m->unfilled = pageV;
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
        double frac = pool->useCount / (float) POOL_PAGE_SIZE;
        averageFillRate += frac;
        printf("Pool %5d:  %3d / %d ",
            i,
            (int) pool->useCount,
            (int) POOL_PAGE_SIZE
        );
        
        if (pool->useCount) {
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
    printf("                 : Overhead   %d KB\n", (int) (m->pages->allocSize * sizeof(mattePoolPage_t))/1024);
}


