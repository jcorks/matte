#include "../matte_string.h"
#include "../matte_vm.h"
#include "../matte_heap.h"
#include "../matte.h"
#include "../matte_compiler.h"
#include "../matte_bytecode_stub.h"
#include "native.h"



#ifdef MATTE_USE_SYSTEM_EXTENSIONS

#ifdef __WIN32__
#define MATTE_IS_WINDOWS

//////////// put the includes needed for all of them here.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h> 
#include <pthread.h>
#include <windows.h>


////////////
#include "./system/winapi/shared.c"
#include "./system/winapi/memorybuffer.c"
#include "./system/winapi/consoleio.c"
#include "./system/winapi/filesystem.c"
#include "./system/winapi/socketio.c"
#include "./system/winapi/time.c"
#include "./system/winapi/utility.c"
#include "./system/winapi/async.c"





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
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h> 
#include <pthread.h>
#include <fcntl.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


////////////
#include "./system/posix/shared.c"
#include "./system/posix/memorybuffer.c"
#include "./system/posix/consoleio.c"
#include "./system/posix/filesystem.c"
#include "./system/posix/socketio.c"
#include "./system/posix/time.c"
#include "./system/posix/utility.c"
#include "./system/posix/async.c"
#else
#error "Unknown system"
#endif





void matte_bind_native_functions(matteVM_t * vm) {
    matte_system__consoleio(vm);
    matte_system__filesystem(vm);
    matte_system__memorybuffer(vm);
    matte_system__socketio(vm);
    matte_system__time(vm);
    matte_system__utility(vm);
    matte_system__async(vm);
    
}




#else 
void matte_bind_native_functions(matteVM_t * vm) {
}
#endif


