@class = import('Matte.Class');
return {
    Server : ::<={
        // Creates a new socket bound to an address and port.
        // args:
        // arg0: address (string). If any, keep blank.
        // arg1: port (number)
        // arg2: type (number)
        //      - 0 => TCP 
        //      - 1 => UDP
        // arg3: maxclients (number)
        // arg4: client timeout in seconds (number)
        // arg5: data mode. 0 for message 1 for raw.
        // 
        // returns:
        // socket server object.
        //
        // CAN THROW AN ERROR if the socket could not be created
        <@>_socket_server_create = getExternalFunction("__matte_ext::socket_server_create");


        // updates the socket server state. This is just:
        // - Accepts incoming, pending clients
        // - Dropping connections that are
        // args:
        // arg0: socket server object
        //
        // returns:
        // None
        //
        <@>_socket_server_update = getExternalFunction("__matte_ext::socket_server_update");


        // Given a client 0-index, returns an ID referring to the client.
        // args:
        // arg0: socket server object 
        // arg1: index 
        // 
        // returns:
        // client id (Number) or empty if not a valid index.
        <@>_socket_server_client_index_to_id



        // Gets the number of clients currently connected.
        // args:
        // arg0: socket server object 
        // 
        // returns:
        // number of clients (Number)
        <@>_socket_server_get_client_count        
        
        
        // Updates the state
        // - Get any additional pending data 
        // - Send out pending data
        // - updates the states of clients
        <@>_socket_server_client_update
        
        // Gets the state of a client 
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // A number flag indicating the state. (Number) 
        //  0 => Disconnected 
        //  1 => Connected
        <@>_socket_server_client_get_state
        
        // Gets a string containing info on the client.
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // info on the client (string)
        <@>_socket_server_client_infostring



        ///////// if in message mode
        // Gets the next completed message
        <@>_socket_server_client_get_next_message
        
        // Sends a new string message. The string 
        // has an upper limit of 16777216 characters
        // per message.
        //
        // args:
        // arg0: socket object 
        // arg1: clientID
        // arg2: message (string)
        // 
        <@>_socket_server_client_send_message
        
        
        
        
        ///////// if in raw mode 
        <@>_socket_server_client_pop_bytes
        
        <@>
        
        
        
        
        
        //
        <@>_socket_server_client_terminate
        


        <@>Client = class({
            define::(this, args, thisclass) {
                <@> events = {};
                this.interface({
                    // event listener
                    on : {
                        get :: {
                            return events;
                        }                
                    },
                    
                    emitEvent ::(ev, data) {
                        @d = events[ev];
                        when(!Boolean(d)) empty;
                        
                        d(data);
                    }
                
                });
            }
        });

        <@>Server = class({
            define::(this, args, thisclass) {
                
                <@>clientCallback = args.onNewClient;            
                when (clientCallback == empty) error('Server must have an "onNewClient" parameter to respond to new connections');
            
                // initialize socket right away.
                <@>socket = ::(
                    address => String,
                    port => Number,
                    maxClients => Number
                ) {
                    return _socket_server_create(address, port, maxClients);
                }(
                    args.address,
                    args.port,
                    0, // => always TCP for now
                    (if (args.maxClients == 0) 128 else args.maxClients),
                    100, // => timeout in seconds. not sure yet!
                );
                
                
                <@>clients = Matte.Array.new();
                
                                
                            
            
                this.interface({
                    update ::{
                        _socket_server_update(socket);  
                        
                        // current client list against prev list.
                        <@>newlen = _socket_server_get_client_count(socket);
                        for([0, newlen], ::(i){
                            
                        }                      
                    }
                });
            }
        });
        
        Server.Client = Client;
        return Server;
    },
    
    
    Client : ::<={

        // Creates a new socket bound to an address and port.
        // args:
        // arg0: address (string)
        // arg1: port (number)
        // arg2: type (number)
        //      - 0 => TCP 
        //      - 1 => UDP
        //
        // CAN THROW AN ERROR if the socket could not be created
        @_socket_create_cl = getExternalFunction("__matte_ext::socket_client_create");


        return class({
            define::(this, args, thisclass) {
                
            }        
        });            
    
    }
};
