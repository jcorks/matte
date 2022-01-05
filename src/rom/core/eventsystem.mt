@class = import(module:'Matte.Core.Class');
@Array = import(module:'Matte.Core.Array');
@EventSystem = class(info:{
    name : 'Matte.Core.EventSystem',
    define::(this) {
        @: events = [];
               
        
        
        this.interface = {
            events : {
                set ::(value) {
                    when (introspect.keycount(of:events) != 0) error(data:'Interface is already defined.');
                    
                    // filter to ensure types of key/val pairs
                    foreach(in:value, do:::(key => String, val => Function) {
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
            emit::(event => String, detail) {
                @: ev = events[event];
                when(ev == empty) error(message:"Cannot emit event for non-existent event "+ev);
                
                @continue = listen(to:::{
                    when(ev.handlerCount == 0) true;
                    for(in:[ev.handlerCount-1, 0], do:::(i) {
                        // when handlers return false, we no longer propogate.           
                        when(!((ev.handlers[i])(detail:detail))) send(message:false);
                    });
                    
                    return true;
                });
                
                // cancelled event. Don't call main handler or hooks.
                when(continue == false) false;
                
                return (if (ev.mainHandler(detail:detail) != false) ::<= {
                    for(in:[0, ev.hookCount], do:::(i) {
                        (ev.hooks[i])(detail:detail);
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
            installHandler ::(event => String, handler => Function) {
                @: ev = events[ev];
                when(ev == empty) error(message:"Cannot install handler for non-existent event "+event);
                
                ev.handlers[ev.handlerCount] = handler;
                ev.handlerCount += 1;
            },
            


            installHook ::(event => String, hook => Function) {
                @: ev = events[event];
                when(ev == empty) error(message:"Cannot install hook for non-existent event "+event);
                
                ev.hooks[ev.hookCount] = hook;
                ev.hookCount += 1;
            },
            
            getKnownEvents ::{
                @arr = Array.new();
                foreach(in:events, do:::(k, v) {
                    arr.push(value:k);
                });
                return arr;                
            },
            
            
            uninstallHook::(event => String, hook) {
                @: ev = events[event];
                when(ev == empty) error(message:"Cannot uninstall hook for non-existent event "+ev);
                listen(to:::{
                    for(in:[0, ev.hookCount], do:::(i) {
                        if (ev.hooks[i] == hook) ::<= { 
                            removeKey(from:ev.hooks, key:i);
                            ev.hookCount-=1;
                            send();
                        };
                    });
                });
            },
            
            uninstallHandler::(event => String, handler) {
                @: ev = events[event];
                when(ev == empty) error(data:"Cannot uninstall handler for non-existent event "+ev);
                listen(to:::{
                    for(in:[0, ev.handlerCount], do:::(i) {
                        if (ev.handlers[i] == handler) ::<= { 
                            removeKey(from:ev.handlers, key:i);
                            ev.handlerCount-=1;
                            send();
                        };
                    });
                });
            }
            
            
            
        };
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




