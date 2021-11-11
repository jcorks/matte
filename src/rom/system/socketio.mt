@class = import('Matte.Core.Class');
@Array = import('Matte.Core.Array');
@EventSystem = import('Matte.Core.EventSystem');
@MemoryBuffer = import('Matte.System.MemoryBuffer');
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
        // arg5: data mode. 0 for raw 1 for message.
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
        <@>_socket_server_client_index_to_id = getExternalFunction("__matte_ext::socket_server_client_index_to_id");



        // Gets the number of clients currently connected.
        // args:
        // arg0: socket server object 
        // 
        // returns:
        // number of clients (Number)
        <@>_socket_server_get_client_count  = getExternalFunction("__matte_ext::socket_server_get_client_count");      
        
        
        // Updates the state
        // - Get any additional pending data 
        // - Send out pending data
        // - updates the states of clients
        <@>_socket_server_client_update = getExternalFunction("__matte_ext::socket_server_client_update");
        
        // Gets the state of a client 
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // A number flag indicating the state. (Number) 
        //  0 => Disconnected 
        //  1 => Connected
        <@>_socket_server_client_get_state = getExternalFunction("__matte_ext::socket_server_client_get_state");
        
        // Gets a string containing the address of the client
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // address of the client (string)
        <@>_socket_server_client_address = getExternalFunction("__matte_ext::socket_server_client_address");

        // Gets a string containing info on the client.
        //
        // args:
        // arg0: socket server object 
        // arg1: client id 
        //
        // returns:
        // info on the client (string)
        <@>_socket_server_client_infostring = getExternalFunction("__matte_ext::socket_server_client_infostring");



        ///////// if in message mode
            // Gets the next completed string message.
            // 
            // args:
            // arg0: socket server object 
            // arg1: client id 
            //
            // returns:
            // string if pending message exists, empty if none.
            <@>_socket_server_client_get_next_message = getExternalFunction("__matte_ext::socket_server_client_get_next_message");
            
            // Sends a new string message. The string 
            // has an upper limit of 16777216 characters
            // per message.
            //
            // args:
            // arg0: socket object 
            // arg1: clientID
            // arg2: message (string)
            // 
            <@>_socket_server_client_send_message = getExternalFunction("__matte_ext::socket_server_client_send_message");
            
            // Gets the number of pending messages that have yet to 
            // be retrieved.
            <@>_socket_server_client_get_pending_message_count = getExternalFunction("__matte_ext::socket_server_client_get_pending_message_count");
        
        
        ///////// if in raw mode 

            // returns:
            // a MemoryBuffer object of the given number of bytes OR throws an error 
            // if cant.
            <@>_socket_server_client_pop_bytes = getExternalFunction("__matte_ext::socket_server_client_pop_bytes");
            
            <@>_socket_server_client_send_bytes = getExternalFunction("__matte_ext::socket_server_client_send_bytes");
            <@>_socket_server_client_get_pending_byte_count = getExternalFunction("__matte_ext::socket_server_client_get_pending_byte_count");



        
        
        
        
        // Request termination. If its successful, the client will be taken off the 
        // managed client list next update.
        <@>_socket_server_client_terminate = getExternalFunction("__matte_ext::socket_server_client_terminate");
        


        // actual client that a user interacts with.
        <@>Client = class({
            inherits:[EventSystem],
            define::(this, args, thisclass) {
                @id = args.id;
                @id_number = Number(id);
                @pendingMessages = {};
                @socket = args.socket;
                @info = _socket_server_client_infostring(args.socket);
                @address = _socket_server_client_address(socket, id_number);



                this.events = {
                    onNewMessage ::{},
                    onIncomingData ::{},
                    onDisconnect ::{}
                };

                // change update based on if a message type client or data
                @update;
                @sendData;
                
                
                if (args.message == true) ::<={
                    update = ::{
                        _socket_server_client_update(socket, id_number);
                        for([0, _socket_server_client_get_pending_message_count(socket, id_number)], ::(i){
                            this.emitEvent(
                                'onNewMessage',
                                _socket_server_client_get_next_message(socket, id_number)
                            );
                        });
                    };

                    sendData = ::(m => String) {
                        _socket_server_client_send_message(socket, id_number, m);
                    };

                } else ::<={
                    update = ::{
                        _socket_server_client_update(socket, id_number);
                        @count = _socket_server_client_get_pending_byte_count(socket, id_number);

                        if (count > 0) ::<={
                            <@> bytes = _socket_server_client_pop_bytes(socket, id_number, count);
                            this.emitEvent(
                                'onIncomingData',
                                bytes
                            );             
                            bytes.release();
                        };
                    };


                    sendData = ::(m => MemoryBuffer) {
                        _socket_server_client_send_bytes(socket, id_number, m);
                    };
                };





                this.interface({
                    update : update,
                    send : sendData,
                    info : {
                        get ::{
                            return info;
                        }
                    },

                    address : {
                        get ::{
                            return address;
                        }
                    },

                    disconnect :: {
                        _socket_server_client_terminate(socket, id_number);
                    }
                });

            }
        });

        <@>Server = class({
            inherits:[EventSystem],
            define::(this, args, thisclass) {

                // initialize socket right away.
                <@>socket = ::(
                    address => String,
                    port => Number,
                    type => Number,
                    maxClients => Number,
                    timeout => Number,
                    messageMode => Boolean
                ) {
                    return _socket_server_create(address, port, type, maxClients, timeout);
                }(
                    args.address,
                    args.port,
                    0, // => always TCP for now
                    (if (args.maxClients == 0) 128 else args.maxClients),
                    100, // => timeout in seconds. not sure yet!
                    Boolean(args.messageMode)
                );
                
                
                <@>clients = Array.new();

                // string id to 
                <@>clientIndex = [];
                                
                this.events = {
                    onNewClient ::{},
                    onDisconnectClient ::{}
                };
            
            
                this.interface({
                    update ::{
                        _socket_server_update(socket);  
                        
                        // current client list against prev list.
                        <@>newlen = _socket_server_get_client_count(socket);
                        <@> found = {};
                        for([0, newlen], ::(i){
                            @id = String(_socket_server_client_index_to_id(socket, i));
                            found[id] = true;

                            // new client
                            if (clientIndex[id] == empty) ::<={
                                @client = Client.new({id:id, socket:socket});
                                clients.push(client);
                                clientIndex[id] = true;

                                this.emitEvent('onNewClient', client);
                            };
                        }); 

                        // emit update disconnects or update.
                        @i = 0;
                        loop(::{
                            when(i == clients.length) false;

                            if (found[clients[i].id]) ::<= {
                                clients[i].update();
                                i+=1;
                            } else ::<={
                                this.emitEvent('onDisconnectClient', clients[i]);
                                removeKey(clients, i);
                                removeKey(clientIndex, clients[i].id);                                
                            };
                            return true;
                        });                  
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



/*

//Server example:

@SocketIO     = import("Matte.System.SocketIO");
@MemoryBuffer = import("Matte.System.MemoryBuffer");


/// hexdump style printing of a memory buffer
<@> dumphex::(data => MemoryBuffer) {
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
        <@> res = asciitable(n);
        when(res == empty) '__';
        return res + ' ';
    };

    return ::{
        print('[' + data.size + ' Bytes]');
        @line       = '';
        @lineAsText = ''; 
        for([0, data.size], ::(i) {
            if (i%8 == 0) ::<={
                print(line + "      " + lineToText);
                line = '';
                lineToText = '';
            };

            line = line + numberToHex(data[i]) + ' ';
            lineToText = lineToText + numberToAscii(data[i])); 
        });
    };
};





@server = SocketIO.Server.new({
    address : '127.0.0.1',
    port : 8080
});




server.installHook('onNewClient', ::(client){
    print('Server: ' + client.address + ' has connected.');

    client.installHook('onDisconnect', ::{
        print('Server: ' + client.address + ' has disconnected');    
    });

    client.installHook('onIncomingData', ::(data) {
        print('Server: ' + client.address + ' has sent ' + data.size + 'bytes.');
        dumphex(data);
    });
});
