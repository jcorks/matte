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
//// Implementation of Stephen Wolfram's 
//// cellular automata

@Time    = import(module:'Matte.System.Time');
@Console = import(module:'Matte.System.ConsoleIO');

@:SWCA_WIDTH = 80;
@:SWCA_SPEED = 120;

// Convert the 3 booleans into a number state
@bit3toNumber::(
    b0 => Boolean,
    b1 => Boolean,
    b2 => Boolean
) {
    return Number(from:b0)  + 
           Number(from:b1)*2 + 
           Number(from:b2)*4;
}

// Populate the next state
@getNextState::(state => Object, index => Number) {
    @before = if (index==0)           SWCA_WIDTH-1 else index-1;
    @after  = if (index==SWCA_WIDTH-1) 0           else index+1;
    

    // Implements "rule 90" 
    return match(bit3toNumber(
        b0:state[before], 
        b1:state[index], 
        b2:state[after]
    )) {
        (0, 4, 7): false,
        default:      true
    }
}

// print the state using 
// different characters per generation
@printState::<={
    @str = '';
    
    @gentable = ['.', ',', '-', ':',
                 ';', 'u', 'o', '%',
                 '#', '@', '8', '=',
                 '+', '^', '"', "'"];
    
    return ::(
        arr => Object,
        generation => Number
    ) {
        str = '';
        @generationStr = gentable[generation%16];
        for(0, SWCA_WIDTH) ::(i){
            str = str + (if(arr[i]) generationStr else ' ');
        }
        return str;
    }
}

@state     = [];
@stateNext = [];


// initialize state
for(0, SWCA_WIDTH) ::(i) {
    state[i] = false;
}


state[SWCA_WIDTH/2] = true;

@generation = 0;
@progress = SWCA_WIDTH;
@str;
 


forever ::{
    // get the full state every line.
    if (progress == SWCA_WIDTH)::<={
        for(0, SWCA_WIDTH) ::(i) {
            @next = getNextState(state:state, index:i);
            stateNext[i] = next;
        }
        str = printState(arr:state, generation:generation);
        generation += 1;
        
        @temp = state;
        state = stateNext;
        stateNext = temp;

        progress = 0;
        Console.printf(format:'\n');
    }
    @nextChar = str->charAt(index:progress);
    @wait = (if (nextChar == ' ') SWCA_SPEED/7 else SWCA_SPEED);
    Time.sleep(milliseconds:Number.random()*wait);
    Console.printf(format:nextChar);
    progress += 1;
}











