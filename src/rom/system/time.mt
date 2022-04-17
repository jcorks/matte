@class = import(module:"Matte.Core.Class");
@EventSystem = import(module:'Matte.Core.EventSystem');


@_sleepms = getExternalFunction(name:"__matte_::time_sleepms");
@_getTicks = getExternalFunction(name:"__matte_::time_getticks");
@initTime = _getTicks();


return class(
    name : 'Matte.System.Time',
    define:::(this) {
        this.interface = {

            // sleeps approximately the given number of milliseconds
            sleep ::(milliseconds => Number){return _sleepms(a:milliseconds);},

            // Gets the current time, as from the C standard time() function with the 
            // NULL argument.
            time : getExternalFunction(name:"__matte_::time_time"),

            // Gets the number of milliseconds since the instance started
            getTicks ::{
                return _getTicks() - initTime;
            }
        };
    }
).new();
