@MatteString = import("Matte.Core.String");
@Array = import('Matte.Core.Array');
@class = import("Matte.Core.Class");
@EventSystem = import('Matte.Core.EventSystem');


@_sleepms = getExternalFunction("__matte_::time_sleepms");
@_getTicks = getExternalFunction("__matte_::time_getticks");
@initTime = _getTicks();

@Clock = class({
    inherits:[EventSystem],
    name : 'Matte.System.Time.Clock',
    
    define::(this) {
        @paused = false;
        @pausedAt;
        @started;
        @period;
        @autoRestart = false;
        
        this.events = {
            onTimeout::{}
        };
        
        
        this.interface({
            autoRestart : {
                get :: {
                    return autoRestart;
                },
                
                set ::(b => Boolean) {
                    autoRestart = b;
                }
            },
            
            
            start::(ms => Number) {
                period = ms;
                started = _getTicks();       
            },

            paused : {
                get :: {
                    return paused;
                },
                
                set ::(b => Boolean) {
                    when(b == paused) empty;
                    paused = b;
                    
                    if (paused)::<= {
                        paused   = true;
                        pausedAt = _getTicks();
                    
                    } else ::<= {
                        paused   = false;
                        started -= (_getTicks() - pausedAt);                                
                    };
                }
            },
            
            timeLeft : {
                get ::{
                    when(paused) (pausedAt - started);
                    <@>left = period - (_getTicks() - started);
                    when(left < 0) 0;
                    return left;
                }
            },
            
            update::{
                when(period == empty) empty;
                when(paused)          empty;
                
                if (_getTicks() - started > period) ::<={
                    this.emitEvent('onTimeout');
                    if (autoRestart) ::<= {
                        this.start(period);
                    };
                };
            }
        });
    }

});


return class({
    name : 'Matte.System.Time',
    define::(this) {
        this.interface({

            // sleeps approximately the given number of milliseconds
            sleep ::(s => Number){return _sleepms(s);},

            // Gets the current time, as from the C standard time() function with the 
            // NULL argument.
            time : getExternalFunction("__matte_::time_time"),

            // Gets the number of milliseconds since the instance of 
            getTicks ::{
                return _getTicks() - initTime;
            },


            /// tasks

            Clock : {
                get :: {
                    return Clock;
                }
            }
        });
    }
}).new();
