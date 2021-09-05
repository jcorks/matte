/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/

#include "matte_string.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#ifdef MATTE_DEBUG
#include <assert.h>
#endif



#define prealloc_size 32


struct matteString_t {
    char * cstr;
    uint32_t len;
    uint32_t alloc;

    matteString_t * delimiters;
    matteString_t * chain;
    uint32_t iter;

    matteString_t * lastSubstr;
};

static void matte_string_concat_cstr(matteString_t * s, const char * cstr, uint32_t len) {
    while (s->len + len + 1 >= s->alloc) {
        s->alloc*=1.4;
        s->cstr = realloc(s->cstr, s->alloc);
    }

    memcpy(s->cstr+s->len, cstr, len+1);
    s->len+=len;
}

static void matte_string_set_cstr(matteString_t * s, const char * cstr, uint32_t len) {
    s->len = 0;
    matte_string_concat_cstr(s, cstr, len);
}




matteString_t * matte_string_create() {
    matteString_t * out = calloc(1, sizeof(matteString_t));
    out->alloc = prealloc_size;
    out->cstr = malloc(prealloc_size);
    out->cstr[0] = 0;
    return out;
}

matteString_t * matte_string_create_from_c_str(const char * format, ...) {
    va_list args;
    va_start(args, format);
    int lenReal = vsnprintf(NULL, 0, format, args);
    va_end(args);


    char * newBuffer = malloc(lenReal+2);
    va_start(args, format);    
    vsnprintf(newBuffer, lenReal+1, format, args);
    va_end(args);
    

    matteString_t * out = matte_string_create();
    matte_string_set_cstr(out, newBuffer, lenReal);
    free(newBuffer);
    return out;
}

matteString_t * matte_string_clone(const matteString_t * src) {
    matteString_t * out = matte_string_create();
    matte_string_set(out, src);
    return out;
}

void matte_string_destroy(matteString_t * s) {
    free(s->cstr);
    if (s->delimiters) matte_string_destroy(s->delimiters);
    if (s->chain) matte_string_destroy(s->chain);
    if (s->lastSubstr) matte_string_destroy(s->lastSubstr);
    free(s);
}

void matte_string_clear(matteString_t * s) {
    s->len = 0;
    s->cstr[0] = 0;
}

void matte_string_set(matteString_t * s, const matteString_t * src) {
    if (s->alloc >= src->alloc) {
        memcpy(s->cstr, src->cstr, src->len+1);
        s->len = src->len;
    } else {
        free(s->cstr);
        s->len = src->len;
        s->alloc = src->alloc;
        s->cstr = malloc(s->alloc);
        memcpy(s->cstr, src->cstr, src->len+1);
    }
    if (s->delimiters) matte_string_destroy(s->delimiters);
    if (s->chain) matte_string_destroy(s->chain);


}



void matte_string_concat_printf(matteString_t * s, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int lenReal = vsnprintf(NULL, 0, format, args);
    va_end(args);


    char * newBuffer = malloc(lenReal+2);
    va_start(args, format);    
    vsnprintf(newBuffer, lenReal+1, format, args);
    va_end(args);
    


    matte_string_concat_cstr(s, newBuffer, lenReal);
    free(newBuffer);
}

void matte_string_concat(matteString_t * s, const matteString_t * src) {
    matte_string_concat_cstr(s, src->cstr, src->len);
}



const matteString_t * matte_string_get_substr(
    const matteString_t * s,
    uint32_t from,
    uint32_t to
) {
    #ifdef MATTE_DEBUG
        assert(from < s->len);
        assert(to < s->len);
    #endif

    if (to < from) {
        uint32_t temp = to;
        to = from;
        from = temp;
    }

    if (!s->lastSubstr) {
        ((matteString_t *)s)->lastSubstr = matte_string_create();
    }

    s->cstr[to+1] = 0;
    matte_string_set_cstr(
        s->lastSubstr, 
        s->cstr+from, 
        (to - from)+1
    );

    return s->lastSubstr;    
}



const char * matte_string_get_c_str(const matteString_t * t) {
    return t->cstr;
}

uint32_t matte_string_get_length(const matteString_t * t) {
    return t->len;
}

int matte_string_get_char(const matteString_t * t, uint32_t p) {
    if (p >= t->len) return 0;
    return t->cstr[p];
}

void matte_string_set_char(matteString_t * t, uint32_t p, int value) {
    if (p >= t->len) return;
    t->cstr[p] = value;
}

void matte_string_append_char(matteString_t * t, int value) {
    char str[2];
    str[1] = 0;
    str[0] = value;
    matte_string_concat_cstr(t, str, 1);
}


uint32_t matte_string_get_byte_length(const matteString_t * t) {
    // for now same as string length. will change when unicode is supported.
    return t->len;
}

void * matte_string_get_byte_data(const matteString_t * t) {
    return t->cstr;
}

int matte_string_test_contains(const matteString_t * a, const matteString_t * b) {
    return strstr(a->cstr, b->cstr) != NULL;
}

int matte_string_test_eq(const matteString_t * a, const matteString_t * b) {
    return strcmp(a->cstr, b->cstr) == 0;
}

int matte_string_compare(const matteString_t * a, const matteString_t * b) {
    return strcmp(a->cstr, b->cstr);
}

matteString_t * matte_string_base64_from_bytes(
    const uint8_t * buffer,
    uint32_t size
) {
    matteString_t * out = matte_string_create();
    const char * key = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint32_t i, j;
    int groupOfSix[4];
    for(i = 0; i < size; i+=3) {
        groupOfSix[0] = 
        groupOfSix[1] = 
        groupOfSix[2] = 
        groupOfSix[3] = -1;

        groupOfSix[0] = buffer[i] >> 2;
        groupOfSix[1] = (buffer[i] & 0x03) << 4;
        if (size > i + 1) {
            groupOfSix[1] |= buffer[i+1] >> 4;
            groupOfSix[2]  = (buffer[i+1] & 0x0f) << 2;
        }
        if (size > i + 2) {
            groupOfSix[2] |= buffer[i+2] >> 6;
            groupOfSix[3] = buffer[i+2] & 0x3f; 
        }

        for(j = 0; j < 4; ++j) {
            if (groupOfSix[j] < 0) {
                matte_string_append_char(out, '=');
            } else {
                matte_string_append_char(out, key[groupOfSix[j]]);
            }
        }
    }
    return out;
}


static char byteKey[] = {
// 0 
    ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', 
// 32
    ' ', ' ', ' ', ' ',    ' ', ' ', ' ', ' ', 
    ' ', ' ', ' ', 62 ,    ' ', ' ', ' ', 63 , 
    52 , 53 , 54 , 55 ,    56 , 57 , 58 , 59 , 
    60 , 61 , ' ', ' ',    ' ', ' ', ' ', ' ', 
// 64
    ' ',  0 ,  1 ,  2 ,     3 ,  4 ,  5 ,  6 , 
     7 ,  8 ,  9 , 10 ,    11 , 12 , 13 , 14 , 
    15 , 16 , 17 , 18 ,    19 , 20 , 21 , 22 , 
    23 , 24 , 25 , ' ',    ' ', ' ', ' ', ' ', 

// 96
    ' ', 26 , 27 , 28 ,    29 , 30 , 31 , 32 , 
    33 , 34 , 35 , 36 ,    37 , 38 , 39 , 40 , 
    41 , 42 , 43 , 44 ,    45 , 46 , 47 , 48 , 
    49 , 50 , 51, ' ',    ' ', ' ', ' ', ' ', 


};

uint8_t * matte_string_base64_to_bytes(
    const matteString_t * in,
    uint32_t * size
) {
    uint32_t i;
    uint32_t len = matte_string_get_length(in);
    uint8_t val;
    if (!len) return NULL;

    *size = 0;
    uint8_t * out = malloc(len); // raw length is always bigger than real length;

    while(len && matte_string_get_char(in, len-1) == '=') len--;
    int buffer = 0;
    int chr;
    int accumulatedBits = 0;
    for(i = 0; i < len; ++i) {
        chr = matte_string_get_char(in, i);
        if (isspace(chr)) continue;
        // invalid string
        if (!(isalnum(chr) || chr == '+' || chr == '/')) {
            *size = 0;
            free(out);
            return NULL;
        }

        buffer <<= 6;
        buffer |= byteKey[chr];
        accumulatedBits += 6;
        if (accumulatedBits == 24) {
            val = (buffer & 0xff0000) >> 16;
            out[(*size)++] = val;
            val = (buffer & 0xff00) >> 8;
            out[(*size)++] = val;
            val = (buffer & 0xff);
            out[(*size)++] = val;
            buffer = 0;
            accumulatedBits = 0;            
        }
    }

    if (accumulatedBits == 12) {
        buffer >>= 4;
        val = buffer;
        out[(*size)++] = val;
    } else if (accumulatedBits == 18) {
        buffer >>= 2;        
        val = (buffer & 0xff00) >> 8;
        out[(*size)++] = val;
        val = (buffer & 0xff);
        out[(*size)++] = val;
    }

    return out;
}




const matteString_t * matte_string_chain_start(matteString_t * t, const matteString_t * delimiters) {
    t->iter = 0;
    if (!t->chain) {
        t->chain = matte_string_create();
        t->delimiters = matte_string_create();
    }
    matte_string_set(t->delimiters, delimiters);
    return matte_string_chain_proceed(t);
}

const matteString_t * matte_string_chain_current(matteString_t * t) {
    if (!t->chain) {
        t->chain = matte_string_create();
        t->delimiters = matte_string_create();
    }
    return t->chain;
}

int matte_string_chain_is_end(const matteString_t * t) {
    return t->iter > t->len;
}

const matteString_t * matte_string_chain_proceed(matteString_t * t) {

    char * del = t->delimiters->cstr;
    char * iter;    

    #define chunk_size 32
    char chunk[chunk_size];
    uint32_t chunkLen = 0;


    char c;

    // skip over leading delimiters
    for(; t->iter < t->len; t->iter++) {
        c = t->cstr[t->iter];

        // delimiter marks the end.
        for(iter = del; *iter; ++iter) {
            if (*iter == c) {
                break;                
            }
        }
        if (!*iter) break;
    }

    // reset for next link
    matte_string_set_cstr(t->chain, "", 0);

    // check if at end
    if (t->iter >= t->len) {
        t->iter = t->len+1;
        return t->chain;
    }

    for(; t->iter < t->len; t->iter++) {
        c = t->cstr[t->iter];

        // delimiter marks the end.
        for(iter = del; *iter; ++iter) {
            if (*iter == c && chunkLen) {
                chunk[chunkLen] = 0;
                matte_string_concat_cstr(t->chain, chunk, chunkLen);                
                return t->chain;
            }
        }         

        if (chunkLen == chunk_size-1) {
            chunk[chunkLen] = 0;
            matte_string_concat_cstr(t->chain, chunk, chunkLen);                
            chunkLen = 0;
        }
        chunk[chunkLen++] = c;
   }

    // reached the end of the string 
    t->iter = t->len;
    chunk[chunkLen] = 0;
    matte_string_concat_cstr(t->chain, chunk, chunkLen);         
    return t->chain;       
}

void matte_string_truncate(
    matteString_t * str,
    uint32_t newLen
) {
    if (newLen < str->len) {
        str->len = newLen;
        str->cstr[newLen] = 0;
    }
}






#define matte_string_temp_max_calls 128
static matteString_t * tempVals[matte_string_temp_max_calls];
static int tempIter = 0;
static int tempInit = 0;

const matteString_t * matte_string_temporary_from_c_str(const char * s) {
    if (!tempInit) {
        uint32_t i;
        for(i = 0; i < matte_string_temp_max_calls; ++i)
            tempVals[i] = matte_string_create();
        tempInit = 1;
    }    

    if (tempIter >= matte_string_temp_max_calls) tempIter = 0;
    matteString_t * t = tempVals[tempIter++];
    matte_string_set_cstr(t, s, strlen(s));
    return t;
}








