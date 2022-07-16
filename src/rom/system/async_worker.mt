@:class = import(module:'Matte.Core.Class');
@:EventSystem = import(module:'Matte.Core.EventSystem');

return [::] {
    // Sends a message to the parent async manager
    // arg0: (string) message to send to parent
    @:_workersendmessage = getExternalFunction(name:"__matte_::asyncworker_send_message");

    // Pulls a new message if present
    // returns: string if a new message. else returns empty
    @:_workercheckmessage = getExternalFunction(name:"__matte_::asyncworker_check_message");
    

    return class(
        name: 'Matte.System.AsyncWorker',
        inherits: [EventSystem],
        define:::(this) {
            this.events = {
                onNewMessage::(detail){}
            };        
            
            this.interface = {
                sendToParent::(message => String) {
                    _workersendmessage(a:message);                
                },
                
                update :: {
                    [::]{
                        forever(do:::{
                            @:msg = _workercheckmessage();        
                            when(msg == empty) send();
                            this.emit(event:'onNewMessage', detail:msg);
                        });
                    };
                }
                
            };
        }
    ).new();
} : {
    onError:::(message) {
        error(detail:'Only workers are allowed to import the AsyncWorker module.');
    }
};

