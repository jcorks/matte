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
};

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
    };
};

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
        [0, SWCA_WIDTH]->for(do:::(i){
            str = str + (if(arr[i]) generationStr else ' ');
        });    
        return str;
    };
};

@state     = [];
@stateNext = [];


// initialize state
[0, SWCA_WIDTH]->for(do:::(i) {
    state[i] = false;
});


state[SWCA_WIDTH/2] = true;

@generation = 0;
@progress = SWCA_WIDTH;
@str;
 


forever(do:::{
    // get the full state every line.
    if (progress == SWCA_WIDTH)::<={
        [0, SWCA_WIDTH]->for(do:::(i) {
            @next = getNextState(state:state, index:i);
            stateNext[i] = next;
        }); 
        str = printState(arr:state, generation:generation);
        generation += 1;
        
        @temp = state;
        state = stateNext;
        stateNext = temp;

        progress = 0;
        Console.printf(format:'\n');
    };
    @nextChar = str->charAt(index:progress);
    @wait = (if (nextChar == ' ') SWCA_SPEED/7 else SWCA_SPEED);
    Time.sleep(milliseconds:Number.random()*wait);
    Console.printf(format:nextChar);
    progress += 1;
});











