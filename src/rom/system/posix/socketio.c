


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
    
    matteArray_t * pendingMessages;
    
    
    // from remote client, delivered.
    matteArray_t * indata;
    
    // to remove client, to be delivered.
    matteArray_t * outdata;
    
    
    
    
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
    
    int mode;
    int poolid;
    
} MatteSocket;




MATTE_EXT_FN(matte_socket__server_create) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    const matteString_t * addr = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    int port = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    int type = matte_value_as_number(matte_array_at(args, matteValue_t, 2));
    int maxClients = matte_value_as_number(matte_array_at(args, matteValue_t, 3));
    double timeout = matte_value_as_number(matte_array_at(args, matteValue_t, 4));
    int mode = matte_value_as_number(matte_array_at(args, matteValue_t, 5));
    // error in args
    if (matte_vm_pending_message(vm)) {
        return matte_heap_new_value(heap);
    }




    MatteSocket listens = {};
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
    
    
    MatteSocket * s = malloc(sizeof(MatteSocket));
    *s = listens;
    s->poolid = 1003;
    s->integID = INTEGRITY_ID_SOCKET_OBJECT;
    s->clients = matte_array_create(sizeof(MatteSocket_Client*));
    
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&out);
    matte_value_object_set_userdata(out, s);
    
    return out;
}


MATTE_EXT_FN(matte_socket__server_update) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocket * s = matte_value_object_get_userdata(v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }
    
    
    // first check for new clients
    int newfd;
    if ((newfd = accept4(s->fd, NULL, NULL)) != -1) {
        MatteSocket_Client * client = calloc(1, sizeof(MatteSocket_Client));
        client->id = s->poolid++;
        client->fd = newfd;    
        assert(s->mode == 0);
        client->indata = matte_array_create(1);
        client->outdata = matte_array_create(1);
        matte_array_push(s->clients, client);
        
    } else {
        const char * err = NULL;
        switch(errno) {
          case EAGAIN:
          case EWOULDBLOCK:
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
    MatteSocket * s = matte_value_object_get_userdata(v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }

    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint32_t size = matte_array_get_size(s->clients);
    if (index >= size) {
        return matte_heap_new_value(heap);    
    }

    MatteSocket_Client * client = matte_array_at(s->clients, MatteSocket_Client *, index);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&v, client->id);
    return v;

}


MATTE_EXT_FN(matte_socket__server_get_client_count) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocket * s = matte_value_object_get_userdata(v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&v, matte_array_get_size(s->clients));
    return v;
}


MATTE_EXT_FN(matte_socket__server_update_client) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_array_at(args, matteValue_t, 0);
    MatteSocket * s = matte_value_object_get_userdata(v);
    if (!s || s->integID != INTEGRITY_ID_SOCKET_OBJECT) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Not a socket server object."));
        return matte_heap_new_value(heap);
    }
    
    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 1));
    uint32_t size = matte_array_get_size(s->clients);
    if (index >= size) {
        return matte_heap_new_value(heap);    
    }

    
    
    
    // next, update all known clients
    uint32_t i;
    uint32_t len = matte_array_get_size(s->clients);
    
    if (s->mode == 0) {
        #define READBLOCKSIZE 512;
        uint8_t block[READBLOCKSIZE];

        MatteSocket_Client * client = matte_array_at(s->clients, MatteSocket_Client *, index);
        ssize_t readsize;
        while((readsize = read(client->fd, block, READBLOCKSIZE)) > 0) {
            matte_array_push_n(client->indata, block, readsize);
        }
        
        if (readsize < 0) {
            const char * err;
            switch(errno) {
              case EAGAIN:
              case EWOULDBLOCK:
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
              default:;
            }            
            
            if (!err) {
                matteString_t * realErr = matte_string_create_from_c_str("Socket read error: Unknown error (%d)", errno);
                matte_vm_raise_error_string(vm, realErr);
                matte_string_destroy(realErr);
            } else {
                matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, err));
            }
        }
        
            
        
        // write any pending data waiting to go out to the client.
        if (matte_array_get_size(client->outdata)) {
            assert(!"Need to implement this");
        }
        
    } else {
        assert(!"Need to implement this");
    }
}


void matte_system__socketio(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_create"),             6, matte_socket__server_create,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_update"),             1, matte_socket__server_update,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_client_index_to_id"), 2, matte_socket__server_client_index_to_id,     NULL);
    matte_vm_set_external_function(vm, MATTE_VM_STR_CAST(vm, "__matte_::socket_server_get_client_count"),   1, matte_socket__server_get_client_count,     NULL);

}





