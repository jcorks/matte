#ifdef MATTE_USE_SYSTEM_EXTENSIONS

#ifdef __WIN32__
#define MATTE_IS_WINDOWS




#elif defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#define MATTE_IS_POSIX

//////////// put the includes needed for all of them here.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h> 
#include <pthread.h>

#include "../../matte_string.h"
#include "../../matte_vm.h"
#include "../../matte_heap.h"
#include "../../matte.h"
#include "../../matte_compiler.h"
#include "../../matte_bytecode_stub.h"

////////////
#include "./posix/shared.c"
#include "./posix/consoleio.c"
#include "./posix/filesystem.c"
#include "./posix/memorybuffer.c"
#include "./posix/socketio.c"
#include "./posix/time.c"
#include "./posix/utility.c"
#include "./posix/async.c"
#else
#error "Unknown system"
#endif


#include "system.h"




void matte_bind_system_functions(matteVM_t * vm) {
    matte_system__consoleio(vm);
    matte_system__filesystem(vm);
    matte_system__memorybuffer(vm);
    matte_system__socketio(vm);
    matte_system__time(vm);
    matte_system__utility(vm);
    matte_system__async(vm);
}




#else 
void matte_bind_system_functions() {
    
}
#endif
