/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
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
#include "../matte_string.h"
#include "../matte_vm.h"
#include "../matte_heap.h"
#include "../matte.h"
#include "../matte_compiler.h"
#include "../matte_bytecode_stub.h"
#include "native.h"


#include "./core/json.c"
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
    matte_system__json(vm);

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
    matte_system__json(vm);


}
#endif


