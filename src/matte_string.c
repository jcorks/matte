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



#define prealloc_size 8


struct matteString_t {
    uint32_t * utf8;
    uint32_t len;
    uint32_t alloc;

    // temporary cstring copy only populated when requested
    char * cstrtemp;
    matteString_t * lastSubstr;
};


static uint32_t utf8_next_char(uint8_t ** source) {
    uint8_t * iter = *source;
    uint32_t val = (*source)[0];
    if (val < 128 && *iter) {
        val = (iter[0]) & 0x7F;
        (*source)++;
        return val;
    } else if (val < 224 && *iter && iter[1]) {
        val = ((iter[0] & 0x1F)<<6) + (iter[1] & 0x3F);
        (*source)+=2;
        return val;
    } else if (val < 240 && *iter && iter[1] && iter[2]) {
        val = ((iter[0] & 0x0F)<<12) + ((iter[1] & 0x3F)<<6) + (iter[2] & 0x3F);
        (*source)+=3;
        return val;
    } else if (*iter && iter[1] && iter[2] && iter[3]) {
        val = ((iter[0] & 0x7)<<18) + ((iter[1] & 0x3F)<<12) + ((iter[2] & 0x3F)<<6) + (iter[3] & 0x3F);
        (*source)+=4;
        return val;
    }
    return 0;
}


static void matte_string_concat_cstr(matteString_t * s, const uint8_t * cstr, uint32_t len) {
    while (s->len + len >= s->alloc) {
        s->alloc*=1.4;
        s->utf8 = realloc(s->utf8, s->alloc*sizeof(uint32_t));
    }

    uint32_t val;
    uint32_t i;
    uint32_t * iter = s->utf8 + s->len; 
    do {
        val = utf8_next_char((uint8_t**)&cstr);
        if (val) {
            s->utf8[s->len++] = val;
        }
    } while(val);
    
    if (s->cstrtemp) {
        free(s->cstrtemp);
        s->cstrtemp = NULL;
    }
}

static void matte_string_set_cstr(matteString_t * s, const uint8_t * cstr, uint32_t len) {
    s->len = 0;
    matte_string_concat_cstr(s, cstr, len);
}




matteString_t * matte_string_create() {
    matteString_t * out = calloc(1, sizeof(matteString_t));
    out->alloc = prealloc_size;
    out->utf8 = malloc(prealloc_size*sizeof(uint32_t));
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
    free(s->cstrtemp);
    free(s->utf8);
    if (s->lastSubstr) matte_string_destroy(s->lastSubstr);
    free(s);
}

void matte_string_clear(matteString_t * s) {
    s->len = 0;
    if (s->cstrtemp) {
        free(s->cstrtemp);
        s->cstrtemp = NULL;
    }
}

void matte_string_set(matteString_t * s, const matteString_t * src) {
    if (s->cstrtemp) {
        free(s->cstrtemp);
        s->cstrtemp = NULL;
    }
    if (s->alloc > src->len) {
        memcpy(s->utf8, src->utf8, src->len*sizeof(uint32_t));
        s->len = src->len;
    } else {
        free(s->utf8);
        s->len = src->len;
        s->alloc = src->len;
        s->utf8 = malloc(s->len*sizeof(uint32_t));
        memcpy(s->utf8, src->utf8, src->len*sizeof(uint32_t));
    }


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
    uint32_t i;
    uint32_t len = src->len;
    for(i = 0; i < len; ++i) {
        matte_string_append_char(s, src->utf8[i]);
    }
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
    if (s->lastSubstr->cstrtemp) {
        free(s->lastSubstr->cstrtemp);
        s->lastSubstr->cstrtemp = NULL;
    }

    uint32_t len = (to - from) + 1;
    if (s->lastSubstr->alloc <= len) {
        s->lastSubstr->alloc = len;
        s->lastSubstr->utf8 = realloc(s->lastSubstr->utf8, len*sizeof(uint32_t));
    }
    memcpy(s->lastSubstr->utf8, s->utf8+from, len*sizeof(uint32_t));
    s->lastSubstr->len = len;
    return s->lastSubstr;    
}



const char * matte_string_get_c_str(const matteString_t * tsrc) {
    if (!tsrc->cstrtemp) {
        matteString_t * t = (matteString_t *)tsrc;
        uint32_t i, n;
        uint32_t len = t->len;
        t->cstrtemp = malloc(len*sizeof(uint32_t)+1);
        uint8_t * iter = t->cstrtemp;
        for(i = 0; i < len; ++i) {
            uint32_t val = t->utf8[i];
            if (val < 0x80) {
                *(iter++) = val & 0x7F;
            } else if (val < 0x800) {
                *(iter++) = ((val & 0x7C0) >> 6) | 0xC0;
                *(iter++) = (val & 0x3F) | 0x80; 
            } else if (val < 0x10000) {
                *(iter++) = ((val & 0xF000) >> 12) | 0xE0; 
                *(iter++) = ((val & 0xFC0) >> 6) | 0x80; 
                *(iter++) = (val & 0x3F) | 0x80; 
            } else {
                *(iter++) = ((val & 0x1C0000) >> 18) | 0xF0; 
                *(iter++) = ((val & 0x3F000) >> 12) | 0x80; 
                *(iter++) = ((val & 0xFC0) >> 6) | 0x80; 
                *(iter++) = (val & 0x3F) | 0x80; 
            }
        }
        *iter = 0;
    }
    return tsrc->cstrtemp;
}

uint32_t matte_string_get_length(const matteString_t * t) {
    return t->len;
}

uint32_t matte_string_get_char(const matteString_t * t, uint32_t p) {
    if (p >= t->len) return 0;
    return t->utf8[p];
}

void matte_string_set_char(matteString_t * t, uint32_t p, uint32_t value) {
    if (p >= t->len) return;
    if (t->cstrtemp) {
        free(t->cstrtemp);
        t->cstrtemp = NULL;
    }
    t->utf8[p] = value;

}

void matte_string_append_char(matteString_t * t, uint32_t value) {
    if (t->len + 1 >= t->alloc) {
        t->alloc*=1.4;
        t->utf8 = realloc(t->utf8, t->alloc*sizeof(uint32_t));
    }
    if (t->cstrtemp) {
        free(t->cstrtemp);
        t->cstrtemp = NULL;
    }

    t->utf8[t->len++] = value;
}

void matte_string_insert_n_chars(
    matteString_t * t,
    uint32_t position,
    uint32_t * values,
    uint32_t nvalues
) {
    if (position >= t->len) return;
    while(t->len + nvalues >= t->alloc) {
        t->alloc*=1.4;
        t->utf8 = realloc(t->utf8, t->alloc*sizeof(uint32_t));
    }
    if (t->cstrtemp) {
        free(t->cstrtemp);
        t->cstrtemp = NULL;
    }

    uint32_t i;
    uint32_t len = nvalues;

    if (position == t->len-1) {
        for(i = 0; i < len; ++i)
            t->utf8[t->len++] = values[i];
    } else {       
        memmove(t->utf8+position+nvalues, t->utf8+position, (t->len-position)*sizeof(uint32_t));
        for(i = 0; i < len; ++i)
            t->utf8[position+i] = values[i];
        t->len += nvalues;
    }
}

void matte_string_remove_n_chars(
    matteString_t * t,
    uint32_t position,
    uint32_t nvalues
) {
    if (position >= t->len) return;
    if (t->cstrtemp) {
        free(t->cstrtemp);
        t->cstrtemp = NULL;
    }
    
    if (position + nvalues >= t->len) {
        matte_string_truncate(t, position);
        return;
    }

    t->len -= nvalues;
    memmove(t->utf8+position, t->utf8+position+nvalues, (t->len - position)*sizeof(uint32_t));
}




uint32_t matte_string_get_byte_length(const matteString_t * t) {
    // for now same as string length. will change when unicode is supported.
    matte_string_get_c_str(t);
    return strlen(t->cstrtemp);
}

void * matte_string_get_byte_data(const matteString_t * t) {
    matte_string_get_c_str(t);
    return t->cstrtemp;
}

int matte_string_test_contains(const matteString_t * a, const matteString_t * b) {
    uint32_t i, n;
    uint32_t * itera, *iterb;
    uint32_t start = b->utf8[0];
    uint32_t ilimit = a->len - b->len;
    if (b->len > a->len) return 0;
    for(i = 0; i < ilimit; ++i) {
        if (a->utf8[i] == start) {
            itera = a->utf8+i;
            iterb = b->utf8;
            ilimit = a->len-i;
            for(n = 0; n < b->len && n < ilimit; ++n, ++itera, ++iterb) {
                if (*itera != *iterb) break;
            }
            if (n == b->len) {
                return 1;
            }
        }
    }
    
    return 0;
}

int matte_string_test_eq(const matteString_t * a, const matteString_t * b) {
    if (a->len != b->len) return 0;
    uint32_t i;
    uint32_t * aiter = a->utf8;
    uint32_t * biter = b->utf8;
    uint32_t len = a->len;
    for(i = 0; i < len; ++i, ++aiter, ++biter) {
        if (*aiter != *biter) return 0;
    }
    return 1;
}

int matte_string_compare(const matteString_t * a, const matteString_t * b) {
    if (a->len < b->len) return -1;
    if (a->len > b->len) return 1;
    uint32_t i;
    uint32_t * aiter = a->utf8;
    uint32_t * biter = b->utf8;
    uint32_t len = a->len;
    for(i = 0; i < len; ++i, ++aiter, ++biter) {
        if (*aiter < *biter) return -1;
        if (*aiter > *biter) return 1;
    }
    return 0;
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





void matte_string_truncate(
    matteString_t * str,
    uint32_t newLen
) {
    if (newLen < str->len) {
        str->len = newLen;
    }
}













