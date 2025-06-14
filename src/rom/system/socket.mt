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
@class = import(:'Matte.Core.Class');
@EventSystem = import(:'Matte.Core.EventSystem');
@MemoryBuffer = import(:'Matte.Core.MemoryBuffer');
@Socket = {
    Server : ::<={
        // Creates a new socket bound to an address and port.
        // args:
        // arg0: in address (string). If any, keep blank.
        // arg1: port (number)
        // arg2: type (number)
        //      - 0 => TCP 
        //      - 1 => UDP
        // arg3: maxclients (number)
        // arg4: client timeout in seconds (number)
        // arg5: data mode. 0 for raw 1 for message.
        // 
        // returns:
        // socket server object.
        //
        // CAN THROW AN ERROR if the socket could not be created
        @:_socket_server_create = getExternalFunction(:"__matte_::socket_server_create");


        // updates the socket server state. This is just:
        // - Accepts incoming, pending clients
        // - Dropping connections that are
        // args:
        // arg0: socket server object
        //
        // returns:
        // None
        //
        @:_socket_server_update = getExternalFunction(:"__matte_::socket_server_update");


        // Given a client 0-index, returns an ID referring to the client.
        // args:
        // arg0: socket server object 
        // arg1: index 
        // 
        // returns:
        // client id (Number) or empty if not a valid index.
        @:_socket_server_client_index_to_id = getExternalFunction(:"__matte_::socket_server_client_index_to_id");



        // Gets the number of clients currently connected.
        // args:
        // arg0: socket server object 
        // 
        // returns:
        // number of clients (Number)
        @:_socket_server_get_client_count  = getExternalFunction(:"__matte_::socket_server_get_client_count");      
        
        
        // Updates the state
        // - Get any additional pending data 
        // - Send out pending data
        // - updates the states of clients
        @:_socket_server_client_update = getExternalFunction(:"__matte_::socket_server_client_update");

        // Gets a string containing the address of the client
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // address of the client (string)
        @:_socket_server_client_address = getExternalFunction(:"__matte_::socket_server_client_address");

        // Gets a string containing info on the client.
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // info on the client (string)
        @:_socket_server_client_infostring = getExternalFunction(:"__matte_::socket_server_client_infostring");



        ///////// if in message mode
            // Gets the next completed string message.
            // 
            // args:
            // arg0: socket server object 
            // arg1: client id 
            //
            // returns:
            // string if pending message exists, empty if none.
            @:_socket_server_client_get_next_message = getExternalFunction(:"__matte_::socket_server_client_get_next_message");
            
            // Sends a new string message. The string 
            // has an upper limit of 16777216 characters
            // per message.
            //
            // args:
            // arg0: socket object 
            // arg1: clientID
            // arg2: message (string)
            // 
            @:_socket_server_client_send_message = getExternalFunction(:"__matte_::socket_server_client_send_message");
            
            // Gets the number of pending messages that have yet to 
            // be retrieved.
            @:_socket_server_client_get_pending_message_count = getExternalFunction(:"__matte_::socket_server_client_get_pending_message_count");
        
        
        ///////// if in raw mode 

            // arg0: server 
            // arg1: client 
            // returns: number of bytes waiting to be read.
            @:_socket_server_client_get_pending_byte_count = getExternalFunction(:"__matte_::socket_server_client_get_pending_byte_count");


            // returns:
            // a MemoryBuffer object of the given number of bytes OR throws an error 
            // if cant.
            @:_socket_server_client_read_bytes = getExternalFunction(:"__matte_::socket_server_client_read_bytes");
            
            @:_socket_server_client_write_bytes = getExternalFunction(:"__matte_::socket_server_client_write_bytes");


            
        
        
        
        
        // Request termination. If its successful, the client will be taken off the 
        // managed client list next update.
        @:_socket_server_client_terminate = getExternalFunction(:"__matte_::socket_server_client_terminate");
        


        // actual client that a user interacts with.
        @:Client = class(
            name: 'Matte.System.Socket.Server.Client',
            inherits:[EventSystem],
            define:::(this) {
                @id_number;
                @pendingMessages = {}
                @socket;
                @info;
                @address;
                @messageIn;


                // change update based on if a message type client or data
                @update;
                @sendData;
                
                
                if (messageIn == true) ::<={
                    update = ::{
                        _socket_server_client_update(a:socket, b:id_number);
                        for(0, _socket_server_client_get_pending_message_count(a:socket, b:id_number))::(i){
                            this.emit(
                                event:'onNewMessage',
                                detail:_socket_server_client_get_next_message(a:socket, b:id_number)
                            );
                        }
                    }

                    sendData = ::(m => String) {
                        _socket_server_client_send_message(a:socket, b:id_number, c:m);
                    }

                } else ::<={
                    update = ::{
                        _socket_server_client_update(a:socket, b:id_number);
                        @count = _socket_server_client_get_pending_byte_count(a:socket, b:id_number);

                        if (count > 0) ::<={
                            @: bytes = MemoryBuffer.new();
                            bytes.bindNative(:_socket_server_client_read_bytes(a:socket, b:id_number));
                            this.emit(
                                event:'onIncomingData',
                                detail:bytes
                            );             
                            bytes.release();
                        }
                    }


                    sendData = ::(bytes => MemoryBuffer.type) {
                        _socket_server_client_write_bytes(a:socket, b:id_number, c:bytes.handle);
                    }
                }




                this.constructor = ::(id, handle, message) {
                    
                    this.events = {
                        onNewMessage ::(detail){},
                        onIncomingData ::(detail){},
                        onDisconnect ::(detail){}
                    }                
                    id_number = Number.parse(:id);
                    socket = handle;
                    messageIn = message;
                    
                    info = _socket_server_client_infostring(a:socket, b:id_number);
                    address = _socket_server_client_address(a:socket, b:id_number);
                };


                this.interface = {        
                
                    update : update,
                    'send' : sendData,
                    info : {
                        get ::{
                            return info;
                        }
                    },

                    id : {
                        get :: {
                            return id_number;
                        }
                    },

                    address : {
                        get ::{
                            return address;
                        }
                    },

                    disconnect :: {
                        _socket_server_client_terminate(a:socket, b:id_number);
                    }
                }

            }
        );

        @:Server = class(
            name: 'Matte.System.Socket.Server',
            inherits:[EventSystem],
            define:::(this) {

                // initialize socket right away.
                @socket;

                
                @:clients = [];

                // string id to 
                @:clientIndex = [];
                                

                this.constructor = ::(restrictAddress, messageMode, port) {
                    this.events = {
                        onNewClient ::(detail){}
                    }

                    socket = ::(
                        address => String,
                        port => Number,
                        type => Number,
                        maxClients => Number,
                        timeout => Number,
                        messageMode => Boolean
                    ) {
                        return _socket_server_create(a:address, b:port, c:type, d:maxClients, e:timeout);
                    } (
                        address:(if (restrictAddress == empty) "" else restrictAddress),
                        port:port,
                        type:0, // => always TCP for now
                        maxClients:128,
                        timeout:100, // => timeout in seconds. not sure yet!
                        messageMode:Boolean(from:messageMode)
                    );
                }

            
            
                this.interface = {

                
                    'Client' : {
                        get ::<- Client
                    },
                
                    update ::{
                        _socket_server_update(a:socket);  
                        
                        // current client list against prev list.
                        @:newlen = _socket_server_get_client_count(a:socket);
                        @: found = {}
                        for(0, newlen)::(i){
                            @id = String(:_socket_server_client_index_to_id(a:socket, b:i));
                            found[id] = true;

                            // new client
                            if (clientIndex[id] == empty) ::<={
                                @client = Client.new(id:id, handle:socket);
                                clients->push(:client);
                                clientIndex[id] = true;
                                found[id] = true;

                                this.emit(event:'onNewClient', detail:client);
                            }
                        } 

                        // emit update disconnects or update.
                        @i = 0;
                        ::? {
                            forever ::{
                                when(i == clients->keycount) send();
                                
                                @idKey = String(:clients[i].id);
                                if (found[idKey]) ::<= {
                                    clients[i].update();
                                    i+=1;
                                } else ::<={
                                    clients[i].emit(event:'onDisconnect', detail:clients[i]);
                                    clients->remove(:i);
                                    clientIndex->remove(:idKey);                                
                                }
                            }
                        }                  
                    }
                }
            }
        );
        
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
        // arg3: mode (number)
        //      - 0 => raw 
        //      - 1 => message
        //
        // CAN THROW AN ERROR if the socket could not be created
        // returns socket handle (number)
        @_socket_client_create = getExternalFunction(:"__matte_::socket_client_create");

        // cleans up the socket, terminating the connection.
        // arg0: socket handle
        @_socket_client_delete = getExternalFunction(:"__matte_::socket_client_delete");

        // arg0: socket handle
        // CAN THROW AN ERROR if the state is 1 (pending connection) and the connection 
        // fails to complete 
        @_socket_client_update = getExternalFunction(:"__matte_::socket_client_update");

        // arg0: socket handle 
        // returns state:
        // 0 -> disconnected 
        // 1 -> pending connection 
        // 2 -> connected
        @_socket_client_get_state = getExternalFunction(:"__matte_::socket_client_get_state");
        
        // arg0: socket handle 
        // returns a string about the host if connected or pending connection.
        @_socket_client_get_host_info = getExternalFunction(:"__matte_::socket_client_get_host_info");
        
        ///////// if in raw mode 
            // arg0: socket handle 
            @_socket_get_pending_byte_count = getExternalFunction(:"__matte_::socket_client_get_pending_byte_count");
    
            // returns:
            // a MemoryBuffer object of the given number of bytes OR throws an error 
            // if cant.
            @:_socket_client_read_bytes = getExternalFunction(:"__matte_::socket_client_read_bytes");
            
            @:_socket_client_write_bytes = getExternalFunction(:"__matte_::socket_client_write_bytes");

                
                
        return class(
            name : 'Matte.System.Socket.Client',
            inherits : [EventSystem],
            define:::(this) {
                @socket;
                

                @state = 0;                
                @update; 
                @sendData;
                
                if(true) ::<={
                    update = ::{
                        when(socket == empty) empty;
                        // raw mode
                        @err;
                        ::?{
                            _socket_client_update(a:socket);
                        } : { 
                            onError:::(message) {
                                @:er = message.detail;
                                err = er;
                            }
                        }

                        @oldstate = state;
                        @newstate = _socket_client_get_state(a:socket);
                        state = newstate;
                        
                        match(true) {
                            (oldstate == 2 && newstate == 0): this.emit(event:'onDisconnect'),
                            (oldstate == 1 && newstate == 0): ::<={
                                this.emit(event:'onConnectFail', detail:err);
                                _socket_client_delete(a:socket);
                                socket = empty;
                            },
                            (oldstate != 2 && newstate == 2): this.emit(event:'onConnectSuccess')
                        }
                        
                        when(state != 2) empty;
                        
                        
                        @count = _socket_get_pending_byte_count(a:socket);                        
                        if (count > 0) ::<={
                            @: bytes = MemoryBuffer.new();
                            bytes.bindNative(handle:_socket_client_read_bytes(a:socket))
                            this.emit(
                                event:'onIncomingData',
                                detail:bytes
                            );             
                            bytes.release();
                        }
                    }

                    sendData = ::(bytes => MemoryBuffer.type) {
                        when(socket == empty) empty;
                        _socket_client_write_bytes(a:socket, b:bytes.handle);
                    }

                } else ::<={
                    error(message:'TODO');                
                }
                
                this.constructor = ::{
                    this.events = {
                        'onConnectSuccess': ::(detail){},
                        'onConnectFail': ::(detail){},
                        'onDisconnect': ::(detail){},
                        
                        'onIncomingData': ::(detail){},
                        'onNewMessage': ::(detail){}
                    };
                };
                
                
                this.interface = {
                    connect::(address => String, port => Number, mode, tls) {
                        when (socket != empty) error(:'Socket is already connected.');
                        if (mode == empty) ::<={
                            mode = 0;
                        }
                        
                        ::? {
                            socket = _socket_client_create(a:address, b:port, c:0, d:mode, e:tls);
                        } : {
                            onError:::(message){
                                this.emit(event:'onConnectFail', detail:message.detail);
                            }
                        }
                    },
                    
                    disconnect::{
                        when(socket == empty) empty;
                        _socket_client_delete(a:socket);
                        socket = empty;              
                    },
                    
                    update : update,
                    'send' : sendData,
                    
                    host : {
                        get :: {
                            when(socket == empty) '';
                            return _socket_client_get_host_info(a:socket);
                        }
                    }              
                }
            }        
        );            
    
    }
}


return Socket;


