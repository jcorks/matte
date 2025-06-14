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
@:class = import(:'Matte.Core.Class');
@:EventSystem = import(:'Matte.Core.EventSystem');

return ::? {
    // Sends a message to the parent async manager
    // arg0: (string) message to send to parent
    @:_workersendmessage = getExternalFunction(:"__matte_::asyncworker_send_message");

    // Pulls a new message if present
    // returns: string if a new message. else returns empty
    @:_workercheckmessage = getExternalFunction(:"__matte_::asyncworker_check_message");
    

    return class(
        name: 'Matte.System.AsyncWorker',
        inherits: [EventSystem],
        define:::(this) {
            this.constructor = ::{
                this.events = {
                    onNewMessage::(detail){}
                }        
            }
            
            this.interface = {
                sendToParent::(message => String) {
                    _workersendmessage(a:message);                
                },
                
                update :: {
                    ::?{
                        forever ::{
                            @:msg = _workercheckmessage();        
                            when(msg == empty) send();
                            this.emit(event:'onNewMessage', detail:msg);
                        }
                    }
                }
                
            }
        }
    ).new();
} : {
    onError:::(message) {
        error(:'Only workers are allowed to import the AsyncWorker module.');
    }
}

