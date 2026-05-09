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
// This demonstrates the server-side basic usage of Socket,
// which performs sockets over IP.
@:Socket = ::? {
    return import(module:'Matte.System.Socket');
} => {
    onError::(message) {
        print(:'ERROR: It looks like the Sockets extension is not compiled in. For security, it is an optional addon. Check the CLI makefiles for the flags to create an executable with socket support.');
    }
}
@:Time         = import(module:'Matte.System.Time');
@:MemoryBuffer = import(module:'Matte.Core.MemoryBuffer');

// Exit if no sockets.
when(Socket == empty) 1;

// This is a utility function for processing
// strings and sending them as MemoryBuffers.
// This will be used below to send data to a connected client.
@sendDataString::(client, str => String) {
    @m = MemoryBuffer.new();
    @:len = str->length; 
    m.size = len;
    for(0, len)::(i){
        m.writeI8(offset:i, value:str->charCodeAt(:i));
    }
    
    client.send(:m);
    m.release();
}











// Servers are initiated as soon as they are created.
// The server requests a port from the OS to use for 
// communication. If the process is not successful,
// an error is thrown.
@server = Socket.Server.new(
    port : 8080
);




// Servers are EventSystems, so most interaction happens 
// as events. The 'onNewClient' is the only event that 
// a server knows.
server.installHook(event:'onNewClient', hook:::(detail){

    // When a new client is specified, its instance is available here.
    // Like the server, clients are EventSystems and interact via events.
    @:client = detail;
    
    // There are also some identifying data members, such as 
    // address.
    print(:'Server: ' + client.address + ' has connected.');


    client.installHooks(:{

        // The onDisconnect event fires when the client is 
        // detected as lost by the OS.
        onDisconnect ::(detail){
            print(:'Server: ' + client.address + ' has disconnected');    
        },
        

        // The onIncomingData event fires when data has successfully 
        // been received from the client.
        onIncomingData ::(detail) {
            
            // Lets assume the data is string data and print it.
            @:data = detail;
            @str = '';
            for(0, data.size)::(i) {
                str = str + ' ';
                str = str->setCharCodeAt(index:i, value:data.readI8(:i));
            }

            print(:'Server: ' + client.address + ' has sent ' + data.size + 
                   'bytes :' + str);        

            // and send back a string to the client
            sendDataString(client:client, str:'pong!');
        }
    });
});

// Now that the events are setup, lets setup a 
// simple loop for the server. The update() function 
// will listen for the server events and fire them as 
// needed.
forever ::{
    server.update();
    Time.sleep(milliseconds:20);
    return true;
}
