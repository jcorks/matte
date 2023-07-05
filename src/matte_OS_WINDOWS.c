////
////
////
// This file implements features that are specific to windows.

#if (_WIN32 || __WIN32__)


#include <windows.h>
double matte_os_get_ticks() {
    LARGE_INTEGER freq = {};
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return (ticks.QuadPart * (1.0 / (freq.QuadPart))) * 1000.0;
}



#endif
