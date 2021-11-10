#ifdef MATTE_USE_SYSTEM_EXTENSIONS
#ifdef __WIN32__
#define MATTE_IS_WINDOWS
#elif __POSIX__
#define MATTE_IS_POSIX
#include "./posix/consoleio.c"
#include "./posix/filesystem.c"
#include "./posix/memorybuffer.c"
#include "./posix/socketio.c"
#include "./posix/time.c"
#include "./posix/utility.c"
#define


void matte_bind_system_functions(matteVM_t * vm) {
    matte_system__consoleio(vm);
    matte_system__filesystem(vm);
    matte_system__memorybuffer(vm);
    matte_system__socketio(vm);
    matte_system__time(vm);
    matte_system__utility(vm);
}




#else 
void matte_bind_system_functions() {
    
}
#endif
