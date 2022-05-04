// This demonstrates the client-side basic usage of Socket,
// which uses sockets over the IP.
@:ConsoleIO = import(module:'Matte.System.ConsoleIO');
@:Socket  = import(module:'Matte.System.Socket');
@:MemoryBuffer = import(module:'Matte.System.MemoryBuffer');
@:Time = import(module:'Matte.System.Time');

// This is a utility function for processing
// strings and sending them as MemoryBuffers.
// This will be used below to send data to a connected server.
@sendDataString::(client, str => String) {
    @m = MemoryBuffer.new();
    @:len = str->length; 
    m.size = len;
    for(in:[0, len], do:::(i){
        m[i] = str->charCodeAt(index:i);
    });
    
    client.send(bytes:m);
};





// This creates a new client instance.
@client = Socket.Client.new();

// Clients are EventSystems, meaning they mostly 
// are interacted with via events and responses to 
// those events.
client.installHooks(events:[
    // When attempting to connect to a server and the connection 
    // is established, the onConnectSuccess event is fired.
    {
        event:'onConnectSuccess', 
        hook::(detail){
            ConsoleIO.println(message:'Successfully connected.');
            
            // Since we were able to connect, lets send some data 
            // to the server.
            sendDataString(client:client, str:'ping!');
        }
    },
    
    // When attempting to connect to a server and the connection 
    // could not be established, the onConnectFail event is fired.
    {
        event:'onConnectFail', 
        hook ::(detail => String) {
            ConsoleIO.println(message:'Connection failed: ' + detail);
        }
    },

    // When an existing connection is broken, this event fires.
    {
        event:'onDisconnect', 
        hook::(detail){
            ConsoleIO.println(message:'Disconnected');
        }
    },
    
    // When the connected server sends data, this event fires,
    // sending with it a MemoryBuffer containing the data.
    {
        event:'onIncomingData', 
        hook::(detail) {
        
            // Lets assume that the data incoming is plain text and print it
            @:data = detail;
            @str = '';
            for(in:[0, data.size], do:::(i) {
                str = str + ' ';
                str = str->setCharCodeAt(index:i, value:data[i]);
            });
            ConsoleIO.println(message:'Message from server: ' + str);
        }
    }
]);


// Once the events are set, we can now 
// connect to the server and have the client 
// emit an event for what happens next.
client.connect(address:'127.0.0.1', port:8080);

// To make sure we get those events, client.update() must 
// be called periodically. Since we're not doing anything 
// else in this program, we can update and sleep.
loop(func:::{    
    client.update();
    Time.sleep(milliseconds:20);
    return true;
});
