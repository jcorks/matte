//// Implementation of Stephen Wolfram's 
//// cellular automata

@Time    = import('Matte.System.Time');
@Console = import('Matte.System.ConsoleIO');
@Utility = import('Matte.System.Utility');
@MString = import('Matte.Core.String');

<@>SWCA_WIDTH = 80;
<@>SWCA_SPEED = 120;

// Convert the 3 booleans into a number state
@bit3toNumber::(
    b0 => Boolean,
    b1 => Boolean,
    b2 => Boolean
) {
    return Number(b0)  + 
           Number(b1)*2 + 
           Number(b2)*4;
};

// Populate the next state
@getNextState::(state => Object, index => Number) {
    @before = if (index==0)           SWCA_WIDTH-1 else index-1;
    @after  = if (index==SWCA_WIDTH-1) 0           else index+1;
    

    // Implements "rule 90" 
    return match(bit3toNumber(
        state[before], 
        state[index], 
        state[after]
    )) {
        (0, 2, 7, 5): false,
        default:      true
    };
};

// print the state using 
// different characters per generation
@printState::<={
    @str = MString.new();
    str.length = SWCA_WIDTH;
    
    @gentable = ['.', ',', '-', ':',
                 ';', 'u', 'o', '%',
                 '#', '@', '8', '=',
                 '+', '^', '"', "'"];
    
    return ::(
        arr => Object,
        generation => Number
    ) {
        @generationStr = gentable[generation%16];
        for([0, SWCA_WIDTH], ::(i){
            str.setCharAt(i, (if(arr[i]) generationStr else ' '));
        });    
        return str;
    };
};

@state     = [];
@stateNext = [];


// initialize state
for([0, SWCA_WIDTH], ::(i) {
    state[i] = false;
});


state[SWCA_WIDTH/2] = true;

@generation = 0;
@progress = SWCA_WIDTH;
@str;
 


loop(::{
    // get the full state every line.
    if (progress == SWCA_WIDTH) ::<= {
        for([0, SWCA_WIDTH], ::(i) {
            @next = getNextState(state, i);
            stateNext[i] = next;
        }); 
        str = printState(state, generation);
        generation += 1;
        
        @temp = state;
        state = stateNext;
        stateNext = temp;

        progress = 0;
        Console.printf('\n');
    };
    @nextChar = String(str.charAt(progress));
    @wait = (if (nextChar == ' ') SWCA_SPEED/7 else SWCA_SPEED);
    Time.sleep(Utility.random*wait);
    Console.printf(nextChar);
    progress += 1;
    return true;
});











