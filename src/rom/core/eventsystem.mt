@class = import('Matte.Class');
@Array = import('Matte.Array');
@EventSystem = class({
    define::(this, args, thisclass) {
        <@> events = [];
               
        
        
        this.interface({
            events : {
                set ::(evs) {
                    when (introspect.keycount(events) != 0) error('Interface is already defined.');
                    
                    // filter to ensure types of key/val pairs
                    foreach(evs, ::(key => String, val => Object) {
                        if (!introspect.isCallable(val)) error('Main event handler must be a function');
                        events[key] = {
                            mainHandler : val,
                            handlers : [],
                            hooks : [],
                            handlerCount : 0,
                            hookCount : 0
                        };
                    });                
                }            
            },
        
            // emits a specific event.
            emitEvent ::(ev => String, data) {
                <@> event = events[ev];
                when(event == empty) error("Cannot emit event for non-existent event "+ev);
                
                @continue = listen(::{
                    when(event.handlerCount == 0) true;
                    for([event.handlerCount-1, 0], ::(i) {
                        // when handlers return false, we no longer propogate.           
                        when(!((event.handlers[i])(this, data))) send(false);
                    });
                    
                    return true;
                });
                
                // cancelled event. Don't call main handler or hooks.
                when(!continue) false;
                
                return (if (event.mainHandler(this, data)) ::<= {
                    for([0, event.hookCount], ::(i) {
                        (event.hooks[i])(this, data);
                    });
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
            installHandler ::(ev => String, fn) {
                <@> event = events[ev];
                when(event == empty) error("Cannot install handler for non-existent event "+ev);
                when(!introspect.isCallable(fn)) error("Cannot install non-callable handler");
                
                event.handlers[event.handlerCount] = fn;
                event.handlerCount += 1;
            },
            


            installHook ::(ev => String, fn) {
                <@> event = events[ev];
                when(event == empty) error("Cannot install hook for non-existent event "+ev);
                when(!introspect.isCallable(fn)) error("Cannot install non-callable hook");
                
                event.hooks[event.hookCount] = fn;
                event.hookCount += 1;
            },
            
            getKnownEvents ::{
                @arr = Array.new();
                foreach(events, ::(k, v) {
                    arr.push(k);
                });
                return arr;                
            },
            
            
            uninstallHook::(ev => String, fn) {
                <@> event = events[ev];
                when(event == empty) error("Cannot uninstall hook for non-existent event "+ev);
                listen(::{
                    for([0, event.hookCount], ::(i) {
                        if (event.hooks[i] == fn) ::<= { 
                            removeKey(event.hooks, i);
                            event.hookCount-=1;
                            send();
                        };
                    });
                });
            },
            
            uninstallHandler::(ev => String, fn) {
                <@> event = events[ev];
                when(event == empty) error("Cannot uninstall handler for non-existent event "+ev);
                listen(::{
                    for([0, event.handlerCount], ::(i) {
                        if (event.handlers[i] == fn) ::<= { 
                            removeKey(event.handlers, i);
                            event.handlerCount-=1;
                            send();
                        };
                    });
                });
            }
            
            
            
        });
    }
});


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
            };
            print('moved from '+ x + ',' + y + ' to '+data.x + ',' + data.y);
            x = data.x;
            y = data.y;          
            // allows hooks to go  
            return true;
        }
    };

    return {
        events : es,
        
        move ::(newx, newy) {
            es.emitEvent('onMove', {x:newx, y:newy});
        }    
    };
};

@hook = ::{
    print('hi!');
};
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
                };
                x = data.x;
                y = data.y;          
                print('moved from '+ x + ',' + y + ' to '+data.x + ',' + data.y);
                // allows hooks to go  
                return true;
            }
        };
        
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




