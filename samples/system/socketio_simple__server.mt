// This demonstrates the server-side basic usage of Socket,
// which performssockets over the IP.
@:Socket     = import(module:'Matte.System.Socket');
@:Time         = import(module:'Matte.System.Time');
@:MemoryBuffer = import(module:'Matte.System.MemoryBuffer');



// This is a utility function for processing
// strings and sending them as MemoryBuffers.
// This will be used below to send data to a connected client.
@sendDataString::(client, str => String) {
    @m = MemoryBuffer.new();
    @:len = String.length(of:str); 
    m.size = len;
    for(in:[0, len], do:::(i){
        m[i] = String.charCodeAt(string:str, index:i);
    });
    
    client.send(bytes:m);
    m.release();
};











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
    print(message:'Server: ' + client.address + ' has connected.');


    client.installHooks(events:[

        // The onDisconnect event fires when the client is 
        // detected as lost by the OS.
        {
            event: 'onDisconnect', 
            hook::(detail){
                print(message:'Server: ' + client.address + ' has disconnected');    
            }
        },
        

        // The onIncomingData event fires when data has successfully 
        // been received from the client.
        {
            event:'onIncomingData', 
            hook::(detail) {
            
                // Lets assume the data is string data and print it.
                @:data = detail;
                @str = '';
                for(in:[0, data.size], do:::(i) {
                    str = str + ' ';
                    str = String.setCharCodeAt(string:str, index:i, value:data[i]);
                });

                print(message:'Server: ' + client.address + ' has sent ' + data.size + 'bytes :' + str);        

                // and send back a string to the client
                sendDataString(client:client, str:'pong!');
            }
        }
    ]);
});

// Now that the events are setup, lets setup a 
// simple loop for the server. The update() function 
// will listen for the server events and fire them as 
// needed.
loop(func:::{
    server.update();
    Time.sleep(milliseconds:20);
    return true;
});
