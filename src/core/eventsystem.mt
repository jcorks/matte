@class = import('Matte.Class');
@Array = import('Matte.Array');
return class.new({
    define::(this, args, thisclass) {
        <@> events = {};
        
        when(args.events == empty) error("EventSystem creation requires that events be installed.");
        
        // filter to ensure types of key/val pairs
        foreach(events, ::(key => String, val => Object) {
            if (!introspect.isCallable(val)) error('Main event handler must be a function');
            events[key] = {
                mainHandler : val,
                handlers : [],
                hooks : [],
                handlerCount : 0,
                hookCount : 0
            };
        });
        
        
        this.interface({
        
            // emits a specific event.
            emitEvent ::(ev => String, data) {
                <@> event = events[ev];
                when(event == empty) error("Cannot emit event for non-existent event "+ev);
                
                @continue = listen(::{
                    for([event.handlerCount-1, 0], ::(i) {
                        // when handlers return false, we no longer propogate.           
                        when(!((event.handlers[i])(this, data))) send(false);
                    });
                    
                    return true;
                });
                
                // cancelled event. Don't call main handler or hooks.
                when(!continue) empty;
                
                if (event.mainHandler(this, data)) ::<= {
                    for([0, event.hookCount], ::(i) {
                        (event.hooks[i])(this, data);
                    });
                };
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
                foreach(events, ::(k, v)) {
                    arr.push(k);
                };
                return arr;                
            },
            
            
            uninstallHook::(ev => String, fn) {
                <@> event = events[ev];
                when(event == empty) error("Cannot uninstall hook for non-existent event "+ev);
                listen(::{
                    for([0, event.hookCount], ::(i) {
                        if (event.hooks[i] == fn) ::<= { 
                            removeKey(event.hooks, i);
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
                            send();
                        };
                    });
                });
            }
            
            
            
        });
    }
});
