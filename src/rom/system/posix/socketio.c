
#include <assert.h>

typedef struct {
    // unique id for the client on this connection
    int id;
    // fd for the connection to the client
    int fd;

    // the connected address.
    matteString_t * address;    

    
    
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
    
    matteArray_t * pendingMessages;
    
    
    // from remote client, delivered.
    matteArray_t * indata;
    
    // to remove client, to be delivered.
    matteArray_t * outdata;
    
    
    // according to documentation online, read() will 
    // return 0 when a disconnect occurs cleanly.
    int connected;
    
    
    
} MatteSocketServer_Client;


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
    
    int mode;
    int poolid;
    
} MatteSocketServer;




MATTE_EXT_FN(matte_socket__server_create) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * addr = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, matte_array_at(args, matteValue_t, 0)));
    int port = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 1));
    int type = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 2));
    int maxClients = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 3));
    double timeout = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 4));
    int mode = matte_value_as_boolean(heap, matte_array_at(args, matteValue_t, 5));
    // error in args
    if (matte_vm_pending_message(vm)) {
        return matte_heap_new_value(heap);
    }




    MatteSocketServer listens = {};
    listens.maxClients = maxClients;    
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
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Given address is not in ipv4 format, ie. xxx.xxx.xxx.xxx"));
            return matte_heap_new_value(heap);
        }
    }
    sIn.sin_port = htons(port); 



    // first create socket that will manage incoming connections

    listens.fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listens.fd == -1) {
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
            matteString_t * realErr = matte_string_create_from_c_str("Socket creation error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
        }
        return matte_heap_new_value(heap);
       
    }
    
    
    // then set up address binding
    if (bind(listens.fd, (struct sockaddr*)&sIn, sizeof(sIn)) == -1) {
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


          //case EACCES:
            //err = "Socket bind error: Search permission is denied on a component of the path prefix";
            //break;

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
            
            
          case ENOTDIR:
            err = "Socket bind error: A component of the path prefix is not a directory.";
            break;
            
          case EROFS:
            err = "Socket bind error: Socket would reside in read-only memory.";
            break;
          
        }
        if (!err) {
            matteString_t * realErr = matte_string_create_from_c_str("Socket bind error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
        }
        return matte_heap_new_value(heap);    
    } 
    
    // now listen for up to max clients
    if (listen(listens.fd, maxClients) == -1) {
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
            matteString_t * realErr = matte_string_create_from_c_str("Socket listen error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
        }
        return matte_heap_new_value(heap);

    } 
    
    
    MatteSocketServer * s = malloc(sizeof(MatteSocketServer));
    *s = listens;
    s->poolid = 1003;
    s->integID = INTEGRITY_ID_SOCKET_OBJECT;
    s->clients = matte_array_create(sizeof(MatteSocketServer_Client*));
    
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &out);
    matte_value_object_set_userdata(heap, out, s);
    
    return out;
}


MATTE_EXT_FN(matte_socket__server_update) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocketServer * s = matte_value_object_get_userdata(heap, v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }
    
    // remove disconnected clients 
    uint32_t i;
    uint32_t len = matte_array_get_size(s->clients);
    for(i = 0; i < len; ++i) {
        MatteSocketServer_Client * client = matte_array_at(s->clients, MatteSocketServer_Client *, i);
        if (!client->connected) {
            if (client->fd) close(client->fd);
            matte_array_destroy(client->indata);
            matte_array_destroy(client->outdata);
            matte_string_destroy(client->address);
            free(client);
            matte_array_remove(s->clients, i);
            i--;   
            len--;
        }
    }
    
    // then check for new clients
    int newfd;
    struct sockaddr_in inaddr = {};
    socklen_t slen = sizeof(struct sockaddr_in);
    if ((newfd = accept(s->fd, &inaddr, &slen)) != -1) {
        char addrString[15];
        MatteSocketServer_Client * client = calloc(1, sizeof(MatteSocketServer_Client));
        if (slen >= sizeof(struct sockaddr_in)) {
            const char * t =  inet_ntoa(inaddr.sin_addr);
            if (t) {
                // protect?
                strcpy(addrString, t);
            } else {
                addrString[0] = 0;            
            }
        } else {
            // ?????
            addrString[0] = 0;
        }
        fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL) | O_NONBLOCK); 


        client->id = s->poolid++;
        client->fd = newfd;    
        client->connected = 1;
        client->address = matte_string_create_from_c_str(addrString);
        assert(s->mode == 0);
        client->indata = matte_array_create(1);
        client->outdata = matte_array_create(1);
        matte_array_push(s->clients, client);
        
    } else {
        const char * err = NULL;
        switch(errno) {
          case EWOULDBLOCK:  // EAGAIN == EWOULDBLOCK
            break; // no need to do anything
            
            
          case EBADF:
            err = "Socket accept error: bad file descriptor";
            break;
          case ECONNABORTED:
            err = "Socket accept error: connection aborted.";
            break;

          case EFAULT:
            err = "Socket accept error: bad address argument";
            break;
            
          case EINTR:
            err = "Socket accept error: system call interrupted before connection could be established.";
            break;
            
          case EINVAL:
            err = "Socket accept error: socket is not listening or address length is invalid.";
            break;

          case EMFILE:
            err = "Socket accept error: system file limit reached.";
            break;
            
          case ENOBUFS:
          case ENOMEM:
            err = "Socket accept error: memory limit reached.";
            break;
            
          case ENOTSOCK:
            err = "Socket accept error: back socket argument.";
            break;

          case EOPNOTSUPP:
            err = "Socket accept error: socket is not of type SOCK_STREAM";
            break;
          case EPERM:
            err = "Socket accept error: Firewall forbids connection.";
            break;
          case EPROTO:
            err = "Socket accept error: protocol error";
            break;
          default:
            err = "Socket accpt error: Unknown error.";
        }
        
        if (err) {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
        }
 
    }

    
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_socket__server_client_index_to_id) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocketServer * s = matte_value_object_get_userdata(heap, v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }

    uint32_t index = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 1));
    uint32_t size = matte_array_get_size(s->clients);
    if (index >= size) {
        return matte_heap_new_value(heap);    
    }

    MatteSocketServer_Client * client = matte_array_at(s->clients, MatteSocketServer_Client *, index);
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, client->id);
    return out;

}


MATTE_EXT_FN(matte_socket__server_get_client_count) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocketServer * s = matte_value_object_get_userdata(heap, v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_number(heap, &out, matte_array_get_size(s->clients));
    return out;
}


MatteSocketServer_Client * id_to_client(matteVM_t * vm, matteValue_t * args, MatteSocketServer ** sout) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    *sout = NULL;
    matteValue_t v = args[0];
    MatteSocketServer * s = matte_value_object_get_userdata(heap, v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return NULL;
    }
    *sout = s;
    
    uint32_t id = matte_value_as_number(heap, args[1]);
    uint32_t size = matte_array_get_size(s->clients);
    uint32_t i;
    for(i = 0; i < size; ++i) {
        MatteSocketServer_Client * client = matte_array_at(s->clients, MatteSocketServer_Client *, i);
        if (client->id == id) {
            return client;
        }        
    }
    return NULL;
}




MATTE_EXT_FN(matte_socket__server_client_update) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    

    if (!client->connected) 
        return matte_heap_new_value(heap);    
        
    
    
    // next, update all known clients
    uint32_t i;
    uint32_t len = matte_array_get_size(s->clients);
    
    if (s->mode == 0) {
        #define READBLOCKSIZE 512
        uint8_t block[READBLOCKSIZE];

        ssize_t readsize;
        while((readsize = read(client->fd, block, READBLOCKSIZE)) > 0) {
            matte_array_push_n(client->indata, block, readsize);
        }
        
        if (readsize < 0) {
            const char * err = NULL;
            switch(errno) {
              case EWOULDBLOCK: // EAGAIN == EWOULDBLOCK
                break;
               
              case EBADF:
                err = "Socket read error: bad file descriptor.";
                break;
              case EFAULT:
                err = "Socket read error: source reading buffer address is invalid.";
                break;
              case EINTR:
                err = "Socket read error: signal interrupted.";
                break;
              case EINVAL:
                err = "Socket read error: file descriptor is not of a type that can be read.";
                break;
              case EIO:
                err = "Socket read error: I/O error. (Please check your hardware!)";
                break;
              case EISDIR:
                err = "Socket read error: the file descriptor points to a directory.";
              default:; {
                matteString_t * realErr = matte_string_create_from_c_str("Socket read error: Unknown error (%d)", errno);
                matte_vm_raise_error_string(vm, realErr);
                matte_string_destroy(realErr);
                break;
              }
            }            
            
            if (err) {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
            }
        } else if (readsize == 0) {
            client->connected = 0;
        }
        
        
            
        
        // write any pending data waiting to go out to the client.
        if (client->connected && matte_array_get_size(client->outdata)) {
            ssize_t writesize;
            while((writesize = write(
                client->fd, 
                matte_array_get_data(client->outdata),
                matte_array_get_size(client->outdata) 
            )) > 0) {
                matte_array_remove_n(client->outdata, 0, writesize);            
            }
            
            if (writesize < 0) {
                const char * err = NULL;
                switch(errno) {
                  case EWOULDBLOCK: // EAGAIN == EWOULDBLOCK
                    break;
                   
                  case EBADF:
                    err = "Socket write error: bad file descriptor.";
                    break;
                  case EDESTADDRREQ:
                    err = "Socket write error: mismatch for socket vs connect. (internal parameter error).";
                    break;
                  case EDQUOT:
                    err = "Socket write error: no space left on the filesystem (quota).";
                    break;
                  case EFAULT:
                    err = "Socket write error: source reading buffer address is invalid.";
                    break;
                  case EFBIG:
                    err = "Socket write error: size too big / out of bounds.";
                    break;
                  case EINTR:
                    err = "Socket write error: signal interrupted.";
                    break;
                  case EINVAL:
                    err = "Socket write error: file descriptor is not of a type that can be read.";
                    break;
                  case ENOSPC:
                    err = "Socket write error: no room left in socket's underlying device.";
                    break;
                  case EPERM:
                    err = "Socket write error: permission error.";
                    break;
                  case EPIPE:
                    err = "Socket write error: other end of pipe was closed.";
                    break;
                  case EIO:
                    err = "Socket write error: I/O error. (Please check your hardware!)";
                    break;
                  case EISDIR:
                    err = "Socket write error: the file descriptor points to a directory.";
                  default:; {
                    matteString_t * realErr = matte_string_create_from_c_str("Socket write error: Unknown error (%d)", errno);
                    matte_vm_raise_error_string(vm, realErr);
                    matte_string_destroy(realErr);
                    break;
                  }
                }            
                
                if (err) {
                    matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
                }
            }
        }
    } else {
        assert(!"Need to implement this");
    }
    return matte_heap_new_value(heap);    
}

MATTE_EXT_FN(matte_socket__server_client_address) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, client->address);
    return out;    
}


MATTE_EXT_FN(matte_socket__server_client_infostring) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(heap, &out, client->address);
    return out;    
}

MATTE_EXT_FN(matte_socket__server_client_terminate) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    if (client->connected) {
        client->connected = 0;
        close(client->fd);
        client->fd = -1;
    }
    
    return matte_heap_new_value(heap);    
        
}

MATTE_EXT_FN(matte_socket__server_client_get_pending_byte_count) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    matteValue_t out = matte_heap_new_value(heap);    
    matte_value_into_number(heap, &out, matte_array_get_size(client->indata));    
    return out;
}

MATTE_EXT_FN(matte_socket__server_client_read_bytes) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }
    matteValue_t m = matte_system_shared__create_memory_buffer_from_raw(
        vm,
        matte_array_get_data(client->indata),
        matte_array_get_size(client->indata)
    );
    matte_array_set_size(client->indata, 0);
    return m; 
}

MATTE_EXT_FN(matte_socket__server_client_write_bytes) {
    MatteSocketServer * s;
    MatteSocketServer_Client * client = id_to_client(vm, matte_array_get_data(args), &s);        
    matteHeap_t * heap = matte_vm_get_heap(vm);

    if (!s || !client) {
        return matte_heap_new_value(heap);    
    }

    uint32_t bufsize;
    const uint8_t * buf = matte_system_shared__get_raw_from_memory_buffer(
        vm,
        matte_array_at(args, matteValue_t, 2),
        &bufsize
    );
    if (!buf) {
        return matte_heap_new_value(heap);        
    }
    
    uint32_t oldsize = matte_array_get_size(client->outdata);
    matte_array_set_size(client->outdata, oldsize+bufsize);
    memcpy(matte_array_get_data(client->outdata)+oldsize, buf, bufsize);
    return matte_heap_new_value(heap);        
}

MATTE_EXT_FN(matte_socket__server_client_get_next_message) {
    assert(!"NOT IMPLEMENTED");
}

MATTE_EXT_FN(matte_socket__server_client_get_pending_message_count) {
    assert(!"NOT IMPLEMENTED");
}

MATTE_EXT_FN(matte_socket__server_client_send_message) {
    assert(!"NOT IMPLEMENTED");
}




typedef struct {
    int socketfd;
    
    // 0 -> data, 1 -> message
    int mode;
    
    // from remote client, delivered.
    matteArray_t * indata;
    
    // to remove client, to be delivered.
    matteArray_t * outdata;
    
    
    // 0-> disconnected / never connected 
    // 1-> pending connection 
    // 2-> connected
    int state;
} MatteSocketClient;

MATTE_EXT_FN(matte_socket__client_create) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * addr = matte_value_string_get_string_unsafe(heap, matte_value_as_string(heap, matte_array_at(args, matteValue_t, 0)));
    int port = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 1));
    int type = matte_value_as_number(heap, matte_array_at(args, matteValue_t, 2));
    int mode = matte_value_as_boolean(heap, matte_array_at(args, matteValue_t, 3));
    // error in args
    if (matte_vm_pending_message(vm)) {
        return matte_heap_new_value(heap);
    }

    struct addrinfo hints = {};
    struct addrinfo * result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; 

    char ps[100];
    sprintf(ps, "%d", port);
    int s = getaddrinfo(matte_string_get_c_str(addr), ps, &hints, &result);

    if (s) {
        matteString_t * errstr = matte_string_create_from_c_str(
            "Socket creation error: address parsing failed: %s\n",
            gai_strerror(s)
        );
        
        matte_vm_raise_error_string(vm, errstr);
        matte_string_destroy(errstr);
        return matte_heap_new_value(heap);
    }
    
    if (!result) {
        matteString_t * errstr = matte_string_create_from_c_str(
            "Socket creation error: no address %s exists or is available\n",
            matte_string_get_c_str(addr)
        );
        matte_vm_raise_error_string(vm, errstr);
        matte_string_destroy(errstr);
        return matte_heap_new_value(heap);        
    }
        


    
    
    int socketfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    
    if (socketfd == -1) {
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
            matteString_t * realErr = matte_string_create_from_c_str("Socket creation error: Unknown error (%d)", errno);
            matte_vm_raise_error_string(vm, realErr);
            matte_string_destroy(realErr);
        } else {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
        }
        freeaddrinfo(result);
        return matte_heap_new_value(heap);
    }
    
    int res = connect(socketfd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    
    if (res != 0) {
        const char * err = NULL;
        switch(errno) {
          case EINPROGRESS:
            // nonblocking, so we need to check progress later to see if we 
            // graduated from a connecting state to a connected state
            break;
            
          case EACCES:
          case EPERM:
            err = "Socket connect error: Permission denied by runtime.";
            break;            
          case EADDRINUSE:
          case EADDRNOTAVAIL:
            err = "Socket connect error: Local address already in use.";
            break;            

          case EAFNOSUPPORT:
            err = "Socket connect error: Unsuported address family.";
            break;            

          case EAGAIN:
            err = "Socket connect error: Internal error (unix socket waiting?)";
            break;            

          case EALREADY:
            err = "Socket connect error: Internal error (connect already in progress?)";
            break;            
          case EBADF:
            err = "Socket connect error: Bad socket.";
            break;            

          case ECONNREFUSED:
            err = "Socket connect error: Connection Refused.";
            break;            

          case EFAULT:
            err = "Socket connect error: Socket structure out of bounds.";
            break;            

          case EISCONN:
            err = "Socket connect error: Socket is already connected.";
            break;
            
          case ENETUNREACH:
            err = "Socket connect error: Network is unreachable.";
            break;
            
          case ENOTSOCK:
            err = "Socket connect error: Not a socket.";
            break;
            
          case EPROTOTYPE:
            err = "Socket connect error: Bad socket type.";
            break;
            
          case ETIMEDOUT:
            err = "Socket connect error: Connection timed out.";
            break;
            
          default:
            err = NULL;
            {
                matteString_t * realErr = matte_string_create_from_c_str("Socket connect error: Unknown error (%d)", errno);
                matte_vm_raise_error_string(vm, realErr);
                matte_string_destroy(realErr);
                close(socketfd);
                return matte_heap_new_value(heap);
            }
          
        
        }    
        if (err) {
            matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
            close(socketfd);
            return matte_heap_new_value(heap);
        } 
    }    
    
    
    MatteSocketClient * cl = calloc(1, sizeof(MatteSocketClient));
    cl->socketfd = socketfd;
    cl->indata = matte_array_create(1);
    cl->outdata = matte_array_create(1);    
    cl->state = res == 0 ? 2 : 1; // if res == 0, then connection was immediate somehow. Probably localhost loop?
    cl->mode = mode;
    
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(heap, &out);
    matte_value_object_set_userdata(heap, out, cl);
    return out;
}


MATTE_EXT_FN(matte_socket__client_delete) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));
    
    if (cl->socketfd) close(cl->socketfd);
    matte_array_destroy(cl->indata);
    matte_array_destroy(cl->outdata);
    free(cl);

    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_socket__client_update) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));
    

    // pending connection... check its status! note: it could fail.
    // on failure, it expects the user code to call the delete function 
    // to cleanup 
    if (cl->state == 1) {
        int status = -1;
        int statusSize = sizeof(int);
        getsockopt(cl->socketfd, SOL_SOCKET, SO_ERROR, &status, &statusSize);
        
        if (status == 0) {
            // connected!!!
            cl->state = 2;
        } else {
            const char * err = NULL;
            switch(status) {
              case EINPROGRESS:
                // nonblocking, so we need to check progress later to see if we 
                // graduated from a connecting state to a connected state
                break;
                
              case EACCES:
              case EPERM:
                err = "Socket connect error: Permission denied by runtime.";
                break;            
              case EADDRINUSE:
              case EADDRNOTAVAIL:
                err = "Socket connect error: Local address already in use.";
                break;            

              case EAFNOSUPPORT:
                err = "Socket connect error: Unsuported address family.";
                break;            

              case EAGAIN:
                err = "Socket connect error: Internal error (unix socket waiting?)";
                break;            

              case EALREADY:
                err = "Socket connect error: Internal error (connect already in progress?)";
                break;            
              case EBADF:
                err = "Socket connect error: Bad socket.";
                break;            

              case ECONNREFUSED:
                err = "Socket connect error: Connection Refused.";
                break;            

              case EFAULT:
                err = "Socket connect error: Socket structure out of bounds.";
                break;            

              case EISCONN:
                err = "Socket connect error: Socket is already connected.";
                break;
                
              case ENETUNREACH:
                err = "Socket connect error: Network is unreachable.";
                break;
                
              case ENOTSOCK:
                err = "Socket connect error: Not a socket.";
                break;
                
              case EPROTOTYPE:
                err = "Socket connect error: Bad socket type.";
                break;
                
              case ETIMEDOUT:
                err = "Socket connect error: Connection timed out.";

                break;
                
              default:
                err = NULL;
                {
                    matteString_t * realErr = matte_string_create_from_c_str("Socket connect error: Unknown error (%d)", errno);
                    matte_vm_raise_error_string(vm, realErr);
                    matte_string_destroy(realErr);
                    cl->state = 0;
                    return matte_heap_new_value(heap);
                }
            }
            
            if (err) {
                cl->state = 0;
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
                return matte_heap_new_value(heap);
            }
        }
    } else if (cl->state == 2) {
        // normal connection case: check for incoming/outgoing data
        if (cl->mode == 0) {
            #define READBLOCKSIZE 512
            uint8_t block[READBLOCKSIZE];

            ssize_t readsize;
            while((readsize = read(cl->socketfd, block, READBLOCKSIZE)) > 0) {
                matte_array_push_n(cl->indata, block, readsize);
            }
            
            if (readsize < 0) {
                const char * err = NULL;
                switch(errno) {
                  case EWOULDBLOCK: // EAGAIN == EWOULDBLOCK
                    break;
                   
                  case EBADF:
                    err = "Socket read error: bad file descriptor.";
                    break;
                  case EFAULT:
                    err = "Socket read error: source reading buffer address is invalid.";
                    break;
                  case EINTR:
                    err = "Socket read error: signal interrupted.";
                    break;
                  case EINVAL:
                    err = "Socket read error: file descriptor is not of a type that can be read.";
                    break;
                  case EIO:
                    err = "Socket read error: I/O error. (Please check your hardware!)";
                    break;
                  case EISDIR:
                    err = "Socket read error: the file descriptor points to a directory.";
                  default:; {
                    matteString_t * realErr = matte_string_create_from_c_str("Socket read error: Unknown error (%d)", errno);
                    matte_vm_raise_error_string(vm, realErr);
                    matte_string_destroy(realErr);
                    break;
                  }
                }            
                
                if (err) {
                    matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
                }
            } else if (readsize == 0) {
                cl->state = 0; // normal disconnect
            }   
            
                
            // write any pending data waiting to go out to the client.
            if (cl->state == 2 && matte_array_get_size(cl->outdata)) {
                ssize_t writesize;
                while((writesize = write(
                    cl->socketfd, 
                    matte_array_get_data(cl->outdata),
                    matte_array_get_size(cl->outdata) 
                )) > 0) {
                    matte_array_remove_n(cl->outdata, 0, writesize);            
                }
                
                if (writesize < 0) {
                    const char * err = NULL;
                    switch(errno) {
                      case EWOULDBLOCK: // EAGAIN == EWOULDBLOCK
                        break;
                       
                      case EBADF:
                        err = "Socket write error: bad file descriptor.";
                        break;
                      case EDESTADDRREQ:
                        err = "Socket write error: mismatch for socket vs connect. (internal parameter error).";
                        break;
                      case EDQUOT:
                        err = "Socket write error: no space left on the filesystem (quota).";
                        break;
                      case EFAULT:
                        err = "Socket write error: source reading buffer address is invalid.";
                        break;
                      case EFBIG:
                        err = "Socket write error: size too big / out of bounds.";
                        break;
                      case EINTR:
                        err = "Socket write error: signal interrupted.";
                        break;
                      case EINVAL:
                        err = "Socket write error: file descriptor is not of a type that can be read.";
                        break;
                      case ENOSPC:
                        err = "Socket write error: no room left in socket's underlying device.";
                        break;
                      case EPERM:
                        err = "Socket write error: permission error.";
                        break;
                      case EPIPE:
                        err = "Socket write error: other end of pipe was closed.";
                        break;
                      case EIO:
                        err = "Socket write error: I/O error. (Please check your hardware!)";
                        break;
                      case EISDIR:
                        err = "Socket write error: the file descriptor points to a directory.";
                      default:; {
                        matteString_t * realErr = matte_string_create_from_c_str("Socket write error: Unknown error (%d)", errno);
                        matte_vm_raise_error_string(vm, realErr);
                        matte_string_destroy(realErr);
                        break;
                      }
                    }            
                    
                    if (err) {
                        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
                    }
                } 
            }
            
            
                     
        } else {
            assert(!"TODO");
        }
    }

    return matte_heap_new_value(heap);
}


MATTE_EXT_FN(matte_socket__client_get_state) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));



    matteValue_t v = matte_heap_new_value(heap);    
    matte_value_into_number(heap, &v, cl->state);
    return v;
}

MATTE_EXT_FN(matte_socket__client_get_host_info) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    matte_value_into_string(heap, &v, MATTE_VM_STR_CAST(vm, ""));
    return v;
}


MATTE_EXT_FN(matte_socket__client_get_pending_byte_count) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    matte_value_into_number(heap, &v, matte_array_get_size(cl->indata));
    return v;
}

MATTE_EXT_FN(matte_socket__client_read_bytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));

    matteValue_t m = matte_system_shared__create_memory_buffer_from_raw(
        vm,
        matte_array_get_data(cl->indata),
        matte_array_get_size(cl->indata)
    );
    matte_array_set_size(cl->indata, 0);
    return m; 
}


MATTE_EXT_FN(matte_socket__client_write_bytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    MatteSocketClient * cl = matte_value_object_get_userdata(heap, matte_array_at(args, matteValue_t, 0));

    uint32_t bufsize;
    const uint8_t * buf = matte_system_shared__get_raw_from_memory_buffer(
        vm,
        matte_array_at(args, matteValue_t, 1),
        &bufsize
    );
    if (!buf) {
        return matte_heap_new_value(heap);        
    }
    
    uint32_t oldsize = matte_array_get_size(cl->outdata);
    matte_array_set_size(cl->outdata, oldsize+bufsize);
    memcpy(matte_array_get_data(cl->outdata)+oldsize, buf, bufsize);
    return matte_heap_new_value(heap);        
}




void matte_system__socketio(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_create"),             6, matte_socket__server_create,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_update"),             1, matte_socket__server_update,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_index_to_id"), 2, matte_socket__server_client_index_to_id,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_get_client_count"),   1, matte_socket__server_get_client_count,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_update"),      2, matte_socket__server_client_update,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_address"),     2, matte_socket__server_client_address,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_infostring"),  2, matte_socket__server_client_infostring,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_terminate"),   2, matte_socket__server_client_terminate,     NULL);

    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_get_pending_byte_count"),   2, matte_socket__server_client_get_pending_byte_count,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_read_bytes"),   2, matte_socket__server_client_read_bytes,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_write_bytes"),   3, matte_socket__server_client_write_bytes,     NULL);

    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_send_message"),   3, matte_socket__server_client_send_message, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_get_next_message"),   2, matte_socket__server_client_get_next_message, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_get_pending_message_count"),   2, matte_socket__server_client_get_pending_message_count, NULL);


    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_create"),  4, matte_socket__client_create, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_delete"),  1, matte_socket__client_delete, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_update"),  1, matte_socket__client_update, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_get_state"),  1, matte_socket__client_get_state, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_get_host_info"),  1, matte_socket__client_get_host_info, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_get_pending_byte_count"),  1, matte_socket__client_get_pending_byte_count, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_read_bytes"),  1, matte_socket__client_read_bytes, NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_client_write_bytes"),  2, matte_socket__client_write_bytes, NULL);

}





