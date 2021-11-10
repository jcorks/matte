#include "ext.h"
#include "../matte_array.h"
#include "../matte_heap.h"
#include "../matte_vm.h"





#ifdef __POSIX__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h> 


typedef struct {
    // unique id for the client on this connection
    int id;
    // fd for the connection to the client
    int fd;
    

    
    
    ////// Reading
    
    // current message id by the client.
    // should always be sequential starting at 1003.
    // allow overflow. 
    uint32_t currentMessageID;
    
    // number of raw bytes encoded in UTF8.
    uint32_t expectedLen;
    
    // current stream. Array of UTF8
    matteArray_t * stream;
    
    
    
    ///// Writing
    /
    matteArray_t * pendingMessages;
    
    
    
} MatteSocket_Client;


#define INTEGRITY_ID_SOCKET_OBJECT 0x080850c3

typedef struct {
    // data integrity ID check.
    uint32_t integID;

    // file descriptor for the listener at the given port
    int fd;
    
    // max clients allowed
    int maxClients;
    
    // size is number fo active clients
    matteArray_t * clients;
    
} MatteSocket;


MATTE_EXT_FN(matte_ext_socket_server_create) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * addr = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    int port = matte_value_as_number(matte_array_at(args, matteValue_t, 1)));
    int type = matte_value_as_number(matte_array_at(args, matteValue_t, 2)));
    int maxClients = matte_value_as_number(matte_array_at(args, matteValue_t, 3)));

    // error in args
    if (matte_vm_pending_message(vm)) {
        return matte_heap_new_value(heap);
    }




    MatteSocket listen = {};
    listen.maxClients = maxClients;    
    struct sockaddr_in sIn = {}; 


    sIn.sin_family = AF_INET;
    
    if (!matte_string_get_length(addr)) {    
        sIn.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(
            AF_INET,
            matte_string_get_c_str(addr),
            &sIn.sin_addr
        ) == 0) {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST("Given address is not in ipv4 format, ie. xxx.xxx.xxx.xxx"));
            return matte_heap_new_value(heap);
        }
    }
    sIn.sin_port = htons(port); 



    // first create socket that will manage incoming connections

    listen.fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen.fd == -1) {
        const char * err = NULL;
        switch(errno) {
          case EACCES:
            err = "Socket creation error: Permission to create a socket of the specified type and/or protocol is denied.";
            break;
            
          case EAFNOSUPPORT:
            err = "Socket creation error: The implementation does not support the specified address family.";
            break;
            
          case EINVAL:
            err = "Socket creation error: Unknown protocol, or protocol family not available.";
            break;
            
          case EMFILE:
            err = "Socket creation error: No more file descriptors can be openned by the process";
            break;
            
          case ENOBUFS:
          case ENOMEM:
            err = "Socket creation error: Inusfficient memory.";
            break;
            
            
          case EPROTONOSUPPORT:
            err = "Socket creation error: Unsupported protocol.";
            break;

          default:;
        }
        if (!err) {
            matteString_r * realErr = matte_string_create_from_c_str("Socket creation error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST(err));
        }
        return matte_heap_new_value(heap);
       
    }
    
    
    // then set up address binding
    if (bind(listen.fd, &sIn, sizeof(struct sockaddr_in) == -1) {
        const char * err = NULL;
        switch(errno) {
          case EACCES: 
            err = "Socket bind error: The address is protected, and the user is not the superuser.";
            break;
                
          case EADDRINUSE:
            err = "Socket bind error: The address is already in use, or no free ports are open at this address.";
            break;

          case EBADF:
            err = "Socket bind error: Bad socket file descriptor";
            break;

          case EINVAL:
            err = "Socket bind error: Socket is already bound to an address";
            break;
            
          case ENOTSOCK:
            err = "Socket bind error: File descriptor is not a socket";
            break;


          case EACCES:
            err = "Socket bind error: Search permission is denied on a component of the path prefix";
            break;

          case EADDRNOTAVAIL:
            err = "Socket bind error: A nonexistent interface was requested or the requested address was not local.";
            break;
            
          case EFAULT:
            err = "Socket bind error: given address points outside the user's accessible address space.";
            break;

          case ELOOP:
            err = "Socket bind error: Too many symbolic links were encountered in resolving the address";
            break;


          case ENAMETOOLONG:
            err = "Socket bind error: address is too long.";
            break;
        
          case ENOENT:
            err = "Socket bind error: A component in the directory prefix of the socket pathname does not exist.";
            break;

          case ENOMEM:
            err = "Socket bind error: Inusfficient memory for operation.";
            break;
            

          case ENOMEM:
            err = "Socket bind error: Inusfficient memory for operation.";
            break;
            
          case ENOTDIR:
            err = "Socket bind error: A component of the path prefix is not a directory.";
            break;
            
          case EROFS:
            err = "Socket bind error: Socket would reside in read-only memory.";
            break;
          
        }
        if (!err) {
            matteString_r * realErr = matte_string_create_from_c_str("Socket bind error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST(err));
        }
        return matte_heap_new_value(heap);    
    } 
    
    // now listen for up to max clients
    if (listen(listen.fd, maxClients) == -1) {
        const char * err = NULL;
        switch(errno) {
          case EADDRINUSE: 
            err = "Socket listen error: Another socket is already listening on the same port or the port was unavailable.";
            break;
            
          case EBADF:
            err = "Socket listen error: Bad file descriptor";
            break; 

          case ENOTSOCK:
            err = "Socket listen error: Not given a file descriptor";
            break; 
    
          case EOPNOTSUPP:
            err = "Socket listen error: This socket does not support the listen operation.";
            break;
        }
        if (!err) {
            matteString_r * realErr = matte_string_create_from_c_str("Socket listen error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_STR_CAST(err));
        }
        return matte_heap_new_value(heap);

    } 
    
    
    MatteSocket * s = malloc(sizeof(MatteSocket));
    *s = listen;
    s->integID = INTEGRITY_ID_SOCKET_OBJECT;
    s->clients = matte_array_create(sizeof(MatteSocket_Client));
    
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&out);
    matte_value_object_set_data(out, s);
    
    return out;
}



void matte_ext__socketio(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_ext::mbuffer_create"),        0, matte_ext__memory_buffer__create,     NULL);

}

#endif 





