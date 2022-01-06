@MatteString = import(module:"Matte.Core.String");
@Array = import(module:'Matte.Core.Array');
@class = import(module:"Matte.Core.Class");
@EventSystem = import(module:'Matte.Core.EventSystem');


@_sleepms = getExternalFunction(name:"__matte_::time_sleepms");
@_getTicks = getExternalFunction(name:"__matte_::time_getticks");
@initTime = _getTicks();

@Clock = class(
    inherits:[EventSystem],
    name : 'Matte.System.Time.Clock',
    
    define:::(this) {
        @paused = false;
        @pausedAt;
        @started;
        @period;
        @autoRestart = false;
        
        this.events = {
            onTimeout::(detail){}
        };
        
        
        this.interface = {
            autoRestart : {
                get :: {
                    return autoRestart;
                },
                
                set ::(b => Boolean) {
                    autoRestart = b;
                }
            },
            
            
            start::(milliseconds => Number) {
                period = milliseconds;
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
                    @:left = period - (_getTicks() - started);
                    when(left < 0) 0;
                    return left;
                }
            },
            
            update::{
                when(period == empty) empty;
                when(paused)          empty;
                
                if (_getTicks() - started > period) ::<={
                    this.emit(event:'onTimeout');
                    if (autoRestart) ::<= {
                        this.start(milliseconds:period);
                    };
                };
            }
        };
    }

);


return class(
    name : 'Matte.System.Time',
    define:::(this) {
        this.interface = {

            // sleeps approximately the given number of milliseconds
            sleep ::(milliseconds => Number){return _sleepms(a:milliseconds);},

            // Gets the current time, as from the C standard time() function with the 
            // NULL argument.
            time : getExternalFunction(name:"__matte_::time_time"),

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
        };
    }
).new();
