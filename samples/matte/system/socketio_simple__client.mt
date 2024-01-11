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
// This demonstrates the client-side basic usage of Socket,
// which uses sockets over the IP.
@:ConsoleIO = import(module:'Matte.System.ConsoleIO');
@:Socket  = import(module:'Matte.System.Socket');
@:MemoryBuffer = import(module:'Matte.Core.MemoryBuffer');
@:Time = import(module:'Matte.System.Time');

// This is a utility function for processing
// strings and sending them as MemoryBuffers.
// This will be used below to send data to a connected server.
@sendDataString::(client, str => String) {
    @m = MemoryBuffer.new();
    @:len = str->length; 
    m.size = len;
    for(0, len)::(i){
        m.writeI8(offset:i, value:str->charCodeAt(index:i));
    }
    
    client.send(bytes:m);
}





// This creates a new client instance.
@client = Socket.Client.new();

// Clients are EventSystems, meaning they mostly 
// are interacted with via events and responses to 
// those events.
client.installHooks(events:{
    // When attempting to connect to a server and the connection 
    // is established, the onConnectSuccess event is fired.
    'onConnectSuccess':::(detail){
        ConsoleIO.println(message:'Successfully connected.');
        
        // Since we were able to connect, lets send some data 
        // to the server.
        sendDataString(client:client, str:'ping!');
    },
    
    // When attempting to connect to a server and the connection 
    // could not be established, the onConnectFail event is fired.
    'onConnectFail':::(detail => String) {
        ConsoleIO.println(message:'Connection failed: ' + detail);
    },

    // When an existing connection is broken, this event fires.
    'onDisconnect':::(detail){
        ConsoleIO.println(message:'Disconnected');
    },
    
    // When the connected server sends data, this event fires,
    // sending with it a MemoryBuffer containing the data.
    'onIncomingData':::(detail) {        
        // Lets assume that the data incoming is plain text and print it
        @:data = detail;
        @str = '';
        for(0, data.size)::(i) {
            str = str + ' ';
            str = str->setCharCodeAt(index:i, value:data.readI8(offset:i));
        }
        ConsoleIO.println(message:'Message from server: ' + str);
    }
});


// Once the events are set, we can now 
// connect to the server and have the client 
// emit an event for what happens next.
client.connect(address:'127.0.0.1', port:8080);

// To make sure we get those events, client.update() must 
// be called periodically. Since we're not doing anything 
// else in this program, we can update and sleep.
forever ::{    
    client.update();
    Time.sleep(milliseconds:20);
    return true;
}
