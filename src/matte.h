#ifndef H_MATTE__CONTEXT__INCLUDED
#define H_MATTE__CONTEXT__INCLUDED


typedef struct matte_t matte_t;
typedef struct matteVM_t matteVM_t;


matte_t * matte_create();

void matte_destroy(matte_t *);

matteVM_t * matte_get_vm(matte_t *);


#endif
