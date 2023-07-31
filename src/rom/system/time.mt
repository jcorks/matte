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
@class = import(module:"Matte.Core.Class");
@EventSystem = import(module:'Matte.Core.EventSystem');


@_sleepms = getExternalFunction(name:"__matte_::time_sleepms");
@_getTicks = getExternalFunction(name:"__matte_::time_getticks");
@_date = getExternalFunction(name:"__matte_::time_date");
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
            },
            
            // gets the current date as an object.
            // The object contains the following members:
            // day. The day of the month 
            // month. 1-indexed month of the year.
            // year. AD.
            date :{
                get::<-_date()
            }
        }
    }
).new();
