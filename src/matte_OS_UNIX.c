////
////
////
// This file implements features that are specific to unix-based OSes.


#if (unix || __unix || __unix__)
#include <sys/time.h>
#include <stddef.h>
double matte_os_get_ticks() {
    struct timeval t; 
    gettimeofday(&t, NULL);
    return t.tv_sec*1000LL + t.tv_usec/1000;
}
    
    
#endif
