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
//// Implementation of Options
//// 

@:class = import(module:'Matte.Core.Class');


// A generator for optional types.
@:Option = class(
    name : 'Option',
    statics : {
        // Convenience function for returning an empty option.
        none :: <- Option.some(),
        
        // The usual method of creating an option with a value.
        some ::(value) <- Option.new(value) 
    },
    
    define::(this) {
        @value_;
        
        this.constructor = ::(value) {
            value_ = value;
        };

        this.interface = {
            map ::(
                some => Function, 
                none => Function
            ) {                    
                when(value_ == empty) none();
                return some(value:value_);
            }        
        };
    }
)



// Example of using the Option, in a similar 
// format to examples on https://en.wikipedia.org/wiki/Option_type

@showValue::(opt => Option.type) <-
    // when "map" is called, the caller must implement what to happen 
    // when the option contains "some" and what happens when the 
    // option contains "none" (AKA empty)
    opt.map(
        some::(value) <- "The value is: " + value,
        none::        <- "No value"
    )
;

@:full    = Option.some(value:42);
@:notFull = Option.none(); 


print(message:"showValue(full)    -> " + showValue(opt:full));
print(message:"showValue(notFull) -> " + showValue(opt:notFull));













