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
@class = import(module:'Matte.Core.Class');
@EventSystem = class(
    name : 'Matte.Core.EventSystem',
    define:::(this) {
        @: events = [];
               
        
        
        this.interface = {
            events : {
                set ::(value) {
                    when (events->keycount != 0) error(detail:'Interface is already defined.');
                    
                    // filter to ensure types of key/val pairs
                    foreach(value)::(key => String, val => Function) {
                        events[key] = {
                            mainHandler : val,
                            handlers : [],
                            hooks : [],
                            handlerCount : 0,
                            hookCount : 0
                        }
                    }                
                }            
            },
        
            // emits a specific event.
            emit::(event => String, detail) {
                @: ev = events[event];
                when(ev == empty) error(detail:"Cannot emit event for non-existent event "+ev);
                
                @continue = {:::} {
                    when(ev.handlerCount == 0) true;
                    for(ev.handlerCount-1, 0)::(i) {
                        // when handlers return false, we no longer propogate.           
                        when(!((ev.handlers[i])(detail:detail))) send(message:false);
                    }
                    
                    return true;
                }
                
                // cancelled event. Don't call main handler or hooks.
                when(continue == false) false;
                
                return (if (ev.mainHandler(detail:detail) != false) ::<= {
                    for(0, ev.hookCount)::(i) {
                        (ev.hooks[i])(detail:detail);
                    }
                    return true;
                } else ::<= {                
                    return false;
                });
            },
            
            
            // installs a new handler. Handlers are meant to be 
            // the main responders to events. They are processed in 
            // reverse order of installation, starting with the last 
            // installed handler. The last handler is always the 
            // main handler, which is installed at creation of the 
            // event system.
            installHandler ::(event => String, handler => Function) {
                @: ev = events[ev];
                when(ev == empty) error(detail:"Cannot install handler for non-existent event "+event);
                
                ev.handlers[ev.handlerCount] = handler;
                ev.handlerCount += 1;
            },
            


            installHook ::(event => String, hook => Function) {
                @: ev = events[event];
                when(ev == empty) error(detail:"Cannot install hook for non-existent event "+event);
                
                ev.hooks[ev.hookCount] = hook;
                ev.hookCount += 1;
            },

            installHooks ::(events => Object) {
                foreach(events)::(event, hook) {
                    this.installHook(event:event, hook:hook);
                }
            },

            
            getKnownEvents ::{
                @arr = [];
                foreach(events) ::(k, v) {
                    arr->push(value:k);
                }
                return arr;                
            },
            
            
            uninstallHook::(event => String, hook) {
                @: ev = events[event];
                when(ev == empty) error(detail:"Cannot uninstall hook for non-existent event "+ev);
                {:::} {
                    for(0, ev.hookCount)::(i) {
                        if (ev.hooks[i] == hook) ::<= { 
                            ev.hooks->remove(key:i);
                            ev.hookCount-=1;
                            send();
                        }
                    }
                }
            },
            
            uninstallHandler::(event => String, handler) {
                @: ev = events[event];
                when(ev == empty) error(detail:"Cannot uninstall handler for non-existent event "+ev);
                {:::} {
                    for(0, ev.handlerCount)::(i) {
                        if (ev.handlers[i] == handler) ::<= { 
                            ev.handlers->remove(key:i);
                            ev.handlerCount-=1;
                            send();
                        }
                    }
                }
            }
            
            
            
        }
    }
);


return EventSystem;

/*
@pointer = ::<={
    @x = 15;
    @y = 15;
    @es = EventSystem.new();
    es.events = {
        'onMove' ::(ev, data) {
            when (data.x < 0 || data.y < 0) ::<= {    
                print('ignored request to move to ' + data.x + ',' + data.y);                    
                return false;
            }
            print('moved from '+ x + ',' + y + ' to '+data.x + ',' + data.y);
            x = data.x;
            y = data.y;          
            // allows hooks to go  
            return true;
        }
    }

    return {
        events : es,
        
        move ::(newx, newy) {
            es.emitEvent('onMove', {x:newx, y:newy});
        }    
    }
}

@hook = ::{
    print('hi!');
}
pointer.events.installHook('onMove', hook);
pointer.move(2, 5);
pointer.move(5, 1);
pointer.move(-1, 10);

pointer.events.uninstallHook('onMove', hook);
pointer.move(1, 4);





@Point = class({
    inherits:[EventSystem],
    define::(this, args) {
    
        @x = 15;
        @y = 15;

        this.events = {
            'onMove' ::(ev, data) {
                when (data.x < 0 || data.y < 0) ::<= {    
                    print('ignored request to move to ' + data.x + ',' + data.y);                    
                    return false;
                }
                x = data.x;
                y = data.y;          
                print('moved from '+ x + ',' + y + ' to '+data.x + ',' + data.y);
                // allows hooks to go  
                return true;
            }
        }
        
        this.interface({
            x : {
                get ::{return x;},
                set ::(val) {
                    this.emitEvent('onMove', {x:val, y:y});
                }
            },

            y : {
                get ::{return y;},
                set ::(val) {
                    this.emitEvent('onMove', {x:x, y:val});
                }
            }

        });
    }
});


@p = Point.new();

p.installHook('onMove', ::{
    print('Moved???');
});

p.x = 10;
p.y = -1;

*/




