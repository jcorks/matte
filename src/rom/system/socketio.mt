@class = import(module:'Matte.Core.Class');
@Array = import(module:'Matte.Core.Array');
@EventSystem = import(module:'Matte.Core.EventSystem');
@MemoryBuffer = import(module:'Matte.System.MemoryBuffer');
@SocketIO = {
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
        <@>_socket_server_create = getExternalFunction(name:"__matte_::socket_server_create");


        // updates the socket server state. This is just:
        // - Accepts incoming, pending clients
        // - Dropping connections that are
        // args:
        // arg0: socket server object
        //
        // returns:
        // None
        //
        <@>_socket_server_update = getExternalFunction(name:"__matte_::socket_server_update");


        // Given a client 0-index, returns an ID referring to the client.
        // args:
        // arg0: socket server object 
        // arg1: index 
        // 
        // returns:
        // client id (Number) or empty if not a valid index.
        <@>_socket_server_client_index_to_id = getExternalFunction(name:"__matte_::socket_server_client_index_to_id");



        // Gets the number of clients currently connected.
        // args:
        // arg0: socket server object 
        // 
        // returns:
        // number of clients (Number)
        <@>_socket_server_get_client_count  = getExternalFunction(name:"__matte_::socket_server_get_client_count");      
        
        
        // Updates the state
        // - Get any additional pending data 
        // - Send out pending data
        // - updates the states of clients
        <@>_socket_server_client_update = getExternalFunction(name:"__matte_::socket_server_client_update");

        // Gets a string containing the address of the client
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // address of the client (string)
        <@>_socket_server_client_address = getExternalFunction(name:"__matte_::socket_server_client_address");

        // Gets a string containing info on the client.
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // info on the client (string)
        <@>_socket_server_client_infostring = getExternalFunction(name:"__matte_::socket_server_client_infostring");



        ///////// if in message mode
            // Gets the next completed string message.
            // 
            // args:
            // arg0: socket server object 
            // arg1: client id 
            //
            // returns:
            // string if pending message exists, empty if none.
            <@>_socket_server_client_get_next_message = getExternalFunction(name:"__matte_::socket_server_client_get_next_message");
            
            // Sends a new string message. The string 
            // has an upper limit of 16777216 characters
            // per message.
            //
            // args:
            // arg0: socket object 
            // arg1: clientID
            // arg2: message (string)
            // 
            <@>_socket_server_client_send_message = getExternalFunction(name:"__matte_::socket_server_client_send_message");
            
            // Gets the number of pending messages that have yet to 
            // be retrieved.
            <@>_socket_server_client_get_pending_message_count = getExternalFunction(name:"__matte_::socket_server_client_get_pending_message_count");
        
        
        ///////// if in raw mode 

            // arg0: server 
            // arg1: client 
            // returns: number of bytes waiting to be read.
            <@>_socket_server_client_get_pending_byte_count = getExternalFunction(name:"__matte_::socket_server_client_get_pending_byte_count");


            // returns:
            // a MemoryBuffer object of the given number of bytes OR throws an error 
            // if cant.
            <@>_socket_server_client_read_bytes = getExternalFunction(name:"__matte_::socket_server_client_read_bytes");
            
            <@>_socket_server_client_write_bytes = getExternalFunction(name:"__matte_::socket_server_client_write_bytes");


            
        
        
        
        
        // Request termination. If its successful, the client will be taken off the 
        // managed client list next update.
        <@>_socket_server_client_terminate = getExternalFunction(name:"__matte_::socket_server_client_terminate");
        


        // actual client that a user interacts with.
        <@>Client = class(definition:{
            name: 'Matte.System.SocketIO.Server.Client',
            inherits:[EventSystem],
            instantiate::(this, thisClass) {
                @id_number;
                @pendingMessages = {};
                @socket;
                @info = _socket_server_client_infostring(a:socket, b:id_number);
                @address = _socket_server_client_address(a:socket, b:id_number);
                @messageIn;

                this.constructor = ::(id, handle, message) {
                    id_number = Number(from:id);
                    socket = handle;
                    messageIn = message;
                };

                this.events = {
                    onNewMessage ::(detail){},
                    onIncomingData ::(detail){},
                    onDisconnect ::(detail){}
                };

                // change update based on if a message type client or data
                @update;
                @sendData;
                
                
                if (messageIn == true) ::<={
                    update = ::{
                        _socket_server_client_update(a:socket, b:id_number);
                        for(in:[0, _socket_server_client_get_pending_message_count(a:socket, b:id_number)], do:::(i){
                            this.emit(
                                event:'onNewMessage',
                                detail:_socket_server_client_get_next_message(a:socket, b:id_number)
                            );
                        });
                    };

                    sendData = ::(m => String) {
                        _socket_server_client_send_message(a:socket, b:id_number, c:m);
                    };

                } else ::<={
                    update = ::{
                        _socket_server_client_update(a:socket, b:id_number);
                        @count = _socket_server_client_get_pending_byte_count(a:socket, b:id_number);

                        if (count > 0) ::<={
                            <@> bytes = MemoryBuffer.new(handle:_socket_server_client_read_bytes(a:socket, b:id_number, c:count));
                            this.emit(
                                event:'onIncomingData',
                                detail:bytes
                            );             
                            bytes.release();
                        };
                    };


                    sendData = ::(m => MemoryBuffer.type) {
                        _socket_server_client_write_bytes(a:socket, b:id_number, c:m.handle);
                    };
                };





                this.interface = {
                    update : update,
                    sendData : sendData,
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
                };

            }
        });

        <@>Server = class(definition:{
            name: 'Matte.System.SocketIO.Server',
            inherits:[EventSystem],
            instantiate::(this, thisClass) {

                // initialize socket right away.
                @socket;
                
                this.constructor = ::(restrictAddress, messageMode, port) {
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
                };

                
                <@>clients = Array.new();

                // string id to 
                <@>clientIndex = [];
                                
                this.events = {
                    onNewClient ::{}
                };
            
            
                this.interface = {
                    update ::{
                        _socket_server_update(a:socket);  
                        
                        // current client list against prev list.
                        <@>newlen = _socket_server_get_client_count(a:socket);
                        <@> found = {};
                        for(in:[0, newlen], do:::(i){
                            @id = String(from:_socket_server_client_index_to_id(a:socket, b:i));
                            found[id] = true;

                            // new client
                            if (clientIndex[id] == empty) ::<={
                                @client = Client.new(id:id, handle:socket);
                                clients.push(value:client);
                                clientIndex[id] = true;
                                found[id] = true;

                                this.emit(name:'onNewClient', detail:client);
                            };
                        }); 

                        // emit update disconnects or update.
                        @i = 0;
                        loop(function:::{
                            when(i == clients.length) false;
                            @idKey = String(from:clients[i].id);
                            if (found[idKey]) ::<= {
                                clients[i].update();
                                i+=1;
                            } else ::<={
                                clients[i].emit(name:'onDisconnect', detail:clients[i]);
                                clients.remove(index:i);
                                removeKey(from:clientIndex, key:idKey);                                
                            };
                            return true;
                        });                  
                    }
                };
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
        // arg3: mode (number)
        //      - 0 => raw 
        //      - 1 => message
        //
        // CAN THROW AN ERROR if the socket could not be created
        // returns socket handle (number)
        @_socket_client_create = getExternalFunction(name:"__matte_::socket_client_create");

        // cleans up the socket, terminating the connection.
        // arg0: socket handle
        @_socket_client_delete = getExternalFunction(name:"__matte_::socket_client_delete");

        // arg0: socket handle
        // CAN THROW AN ERROR if the state is 1 (pending connection) and the connection 
        // fails to complete 
        @_socket_client_update = getExternalFunction(name:"__matte_::socket_client_update");

        // arg0: socket handle 
        // returns state:
        // 0 -> disconnected 
        // 1 -> pending connection 
        // 2 -> connected
        @_socket_client_get_state = getExternalFunction(name:"__matte_::socket_client_get_state");
        
        // arg0: socket handle 
        // returns a string about the host if connected or pending connection.
        @_socket_client_get_host_info = getExternalFunction(name:"__matte_::socket_client_get_host_info");
        
        ///////// if in raw mode 
            // arg0: socket handle 
            @_socket_get_pending_byte_count = getExternalFunction(name:"__matte_::socket_client_get_pending_byte_count");
    
            // returns:
            // a MemoryBuffer object of the given number of bytes OR throws an error 
            // if cant.
            <@>_socket_client_read_bytes = getExternalFunction(name:"__matte_::socket_client_read_bytes");
            
            <@>_socket_client_write_bytes = getExternalFunction(name:"__matte_::socket_client_write_bytes");

                
                
        return class(definition:{
            name : 'Matte.System.SocketIO.Client',
            inherits : [EventSystem],
            instantiate::(this, thisClass) {
                @socket;
                
                this.events = {
                    'onConnectSuccess': ::(detail){},
                    'onConnectFail': ::(detail){},
                    'onDisconnect': ::(detail){},
                    
                    'onIncomingData': ::(detail){},
                    'onNewMessage': ::(detail){}
                };
                @state = 0;                
                @update; 
                @sendData;
                
                if(true) ::<={
                    update = ::{
                        when(socket == empty) empty;
                        // raw mode
                        @err;
                        listen(to:::{
                            _socket_client_update(a:socket);
                        }, onMessage:::(er) {
                            err = er;
                        });

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
                            (oldstate != 2 && newstate == 2): this.emit(name:'onConnectSuccess')
                        };
                        
                        when(state != 2) empty;
                        
                        
                        @count = _socket_get_pending_byte_count(a:socket);                        
                        if (count > 0) ::<={
                            <@> bytes = MemoryBuffer.new(handle:_socket_client_read_bytes(a:socket, b:count));
                            this.emit(
                                name:'onIncomingData',
                                detail:bytes
                            );             
                            bytes.release();
                        };
                    };

                    sendData = ::(m => MemoryBuffer.type) {
                        when(socket == empty) empty;
                        _socket_client_write_bytes(a:socket, b:m.handle);
                    };

                } else ::<={
                    error(message:'TODO');                
                };
                
                
                
                
                this.interface = {
                    connect::(addr => String, port => Number, mode) {
                        when (socket != empty) error(message:'Socket is already connected.');
                        if (mode == empty) ::<={
                            mode = 0;
                        };
                        
                        listen(to:::{
                            socket = _socket_client_create(a:addr, b:port, c:0, d:mode);
                        }, onMessage:::(message){
                            this.emit(event:'onConnectFail', detail:message);
                        });
                    },
                    
                    disconnect::{
                        when(socket == empty) empty;
                        _socket_client_delete(a:socket);
                        socket = empty;              
                    },
                    
                    update : update,
                    sendData : sendData,
                    
                    host : {
                        get :: {
                            when(socket == empty) '';
                            return _socket_client_get_host_info(a:socket);
                        }
                    }              
                };
            }        
        });            
    
    }
};


return SocketIO;



/*

//@SocketIO     = import("Matte.System.SocketIO");
//@MemoryBuffer = import("Matte.System.MemoryBuffer");
@Array = import("Matte.Core.Array");
@Time = import("Matte.System.Time");
@ConsoleIO = import("Matte.System.ConsoleIO");

/// hexdump style printing of a memory buffer
<@> dumphex = ::<={
    @hextable = {
        0: '0',
        1: '1',
        2: '2',
        3: '3',
        4: '4',
        5: '5',
        6: '6',
        7: '7',
        8: '8',
        9: '9',
        10: 'a',
        11: 'b',
        12: 'c',
        13: 'd',
        14: 'e',
        15: 'f'
    };

    // assumes 0-255
    <@> numberToHex::(n => Number) {
        return hextable[n / 16] + // number lookups are always floored.
               hextable[n % 16];
    };

    @asciitable = {
        33: '!',
        34: '"',
        35: '#',
        36: '$',
        37: '%',
        38: '&',
        39: "'",
        40: '(',
        41: ')',
        42: '*',
        43: '+',
        44: ',',
        45: '-',
        46: '.',
        47: '/',
        48: '0',
        49: '1',
        50: '2',
        51: '3',
        52: '4',
        53: '5',
        54: '6',
        55: '7',
        56: '8',
        57: '9',
        58: ':',
        59: ';',
        60: '<',
        61: '=',
        62: '>',
        63: '?',
        64: '@',
        65: 'A',
        66: 'B',
        67: 'C',
        68: 'D',
        69: 'E',
        70: 'F',
        71: 'G',
        72: 'H',
        73: 'I',
        74: 'J',
        75: 'K',
        76: 'L',
        77: 'M',
        78: 'N',
        79: 'O',
        80: 'P',
        81: 'Q',
        82: 'R',
        83: 'S',
        84: 'T',
        85: 'U',
        86: 'V',
        87: 'W',
        88: 'X',
        89: 'Y',
        90: 'Z',
        91: '[',
        92: '\\',
        93: ']',
        94: '^',
        95: '_',
        96: '`',
        97: 'a',
        98: 'b',
        99: 'c',
        100: 'd',
        101: 'e',
        102: 'f',
        103: 'g',
        104: 'h',
        105: 'i',
        106: 'j',
        107: 'k',
        108: 'l',
        109: 'm',
        110: 'n',
        111: 'o',
        112: 'p',
        113: 'q',
        114: 'r',
        115: 's',
        116: 't',
        117: 'u',
        118: 'v',
        119: 'w',
        120: 'x',
        121: 'y',
        122: 'z',
        123: '{',
        124: '|',
        125: '}',
        126: '~'
    };

    <@> numberToAscii::(n => Number) {
        <@> res = asciitable[n];
        when(res == empty) '__';
        return res + ' ';
    };

    return ::(data => MemoryBuffer.type){
        ConsoleIO.println('[' + data.size + ' Bytes]');
        @line       = '';
        @lineAsText = ''; 
        for([0, data.size], ::(i) {
            if (i%8 == 0) ::<={
                ConsoleIO.println(line + "      " + lineAsText);
                line = '';
                lineAsText = '';
            };

            line = line + numberToHex(data[i]) + ' ';
            lineAsText = lineAsText + numberToAscii(data[i]); 
        });
        
        if (lineAsText != '') ::<= {
            ConsoleIO.println(line + "      " + lineAsText);            
        };
    };
};

@sendDataString::(client, str => String) {
    @m = MemoryBuffer.new();
    <@>len = introspect.length(str); 
    m.size = len;
    for([0, len], ::(i){
        m[i] = introspect.charCodeAt(str, i);
    });
    
    client.sendData(m);
};
*/





//client example:
/*
@client = SocketIO.Client.new();

client.installHook('onConnectSuccess', ::{
    ConsoleIO.println('Successfully connected.');
    sendDataString(client, 'whoa!!');
});

client.installHook('onConnectFail', ::(v, reason => String) {
    ConsoleIO.println('Connection failed: ' + String(reason));
});

client.installHook('onDisconnect', ::{
    ConsoleIO.println('Disconnected');
});

client.installHook('onIncomingData', ::(v, data => MemoryBuffer.type) {
    @a = [];
    for([0, data.size], ::(i) {
        a[i] = data[i];
    });
    @str = introspect.arrayToString(a);
    ConsoleIO.println('Message from server: ' + str);
});

client.connect('127.0.0.1', 8080);
loop(::{    
    client.update();
    Time.sleep(20);
    return true;
});
*/


//Server example:


/*
@server = SocketIO.Server.new({
    port : 8080
});



server.installHook('onNewClient', ::(t, client){
    print('Server: ' + client.address + ' has connected.');

    client.installHook('onDisconnect', ::{
        print('Server: ' + client.address + ' has disconnected');    
    });

    client.installHook('onIncomingData', ::(t, data) {
        print('Server: ' + client.address + ' has sent ' + data.size + 'bytes.');
        dumphex(data);
        sendDataString(client, 'whoa!!');
    });
});


loop(::{
    server.update();
    Time.sleep(20);

    return true;
});

*/