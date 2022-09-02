@MemoryBuffer = import(module:'Matte.System.MemoryBuffer');
@ConsoleIO    = import(module:'Matte.System.ConsoleIO');
@class        = import(module:'Matte.Core.Class');


when(parameters == empty || parameters.file == empty) ::<={
    print(message:'DumpHex usage:');
    print(message:'');
    print(message:'matte DumpHex.mt file:[filename]');

};


// config
@:BYTES_PER_LINE = if ((parameters.wordsize)->type == String) Number.parse(string:parameters.wordsize) else 8;
@:BYTES_PER_PAGE = BYTES_PER_LINE*8;
    

@hextable = {
    0: '0',
    1: '1',
    2: '2',
    3: '3',
    4: '4',
    5: '5',
    6: '6',
    7: '7',
    8: '8',
    9: '9',
    10: 'a',
    11: 'b',
    12: 'c',
    13: 'd',
    14: 'e',
    15: 'f'
};

// assumes 0-255
@: numberToHex::(n => Number) {
    return {first :hextable[n / 16], // number lookups are always floored.
     second:hextable[n % 16]};
};

@asciitable = {
    33: '!',
    34: '"',
    35: '#',
    36: '$',
    37: '%',
    38: '&',
    39: "'",
    40: '(',
    41: ')',
    42: '*',
    43: '+',
    44: ',',
    45: '-',
    46: '.',
    47: '/',
    48: '0',
    49: '1',
    50: '2',
    51: '3',
    52: '4',
    53: '5',
    54: '6',
    55: '7',
    56: '8',
    57: '9',
    58: ':',
    59: ';',
    60: '<',
    61: '=',
    62: '>',
    63: '?',
    64: '@',
    65: 'A',
    66: 'B',
    67: 'C',
    68: 'D',
    69: 'E',
    70: 'F',
    71: 'G',
    72: 'H',
    73: 'I',
    74: 'J',
    75: 'K',
    76: 'L',
    77: 'M',
    78: 'N',
    79: 'O',
    80: 'P',
    81: 'Q',
    82: 'R',
    83: 'S',
    84: 'T',
    85: 'U',
    86: 'V',
    87: 'W',
    88: 'X',
    89: 'Y',
    90: 'Z',
    91: '[',
    92: '\\',
    93: ']',
    94: '^',
    95: '_',
    96: '`',
    97: 'a',
    98: 'b',
    99: 'c',
    100: 'd',
    101: 'e',
    102: 'f',
    103: 'g',
    104: 'h',
    105: 'i',
    106: 'j',
    107: 'k',
    108: 'l',
    109: 'm',
    110: 'n',
    111: 'o',
    112: 'p',
    113: 'q',
    114: 'r',
    115: 's',
    116: 't',
    117: 'u',
    118: 'v',
    119: 'w',
    120: 'x',
    121: 'y',
    122: 'z',
    123: '{',
    124: '|',
    125: '}',
    126: '~'
};









@: numberToAscii::(n => Number) {
    @: res = asciitable[n];
    when(res == empty) {
        first: '_',
        second: '_'
    };
    return {
        first: res,
        second: ' '
    };
};

@line;
@lineAsText = '';




@: dumphex ::(data => MemoryBuffer.type, onPageFinish => Function){
    line = '';
    [0, BYTES_PER_LINE*2+BYTES_PER_LINE]->for(do:::(i) {
        line = line + ' ';
    });    
    lineAsText = '';
    [0, BYTES_PER_LINE*2]->for(do:::(i) {
        lineAsText = lineAsText + ' ';
    });    



    @iterBytes = 0;
    
    @:endPoint :: {
        when(iterBytes+BYTES_PER_PAGE >= data.size) data.size;
        return iterBytes+BYTES_PER_PAGE;
    };

    [::]{
        forever(do:::{
            @iter = 0;
            @lineIter = 0;
            @lineAsTextIter = 0;
            @out = '';
            @lines = [];

        
            [iterBytes, endPoint()]->for(do:::(i) {
                if (i%BYTES_PER_LINE == 0) ::<={
                    lines->push(value: '' + line + "      " + lineAsText + '\n');
                    iter = 0;
                    line = '';
                    lineAsText = '';
                };

                @n = numberToHex(n:data[i]);
                line = String.combine(strings:[line, n.first, n.second, ' ']);

                n = numberToAscii(n:data[i]);
                lineAsText = String.combine(strings:[lineAsText, n.first, n.second]);  

                iter += 1;
            });
            
            if (iter%BYTES_PER_LINE) ::<= {
                [iter, BYTES_PER_LINE]->for(do:::(i){
                    @n = numberToHex(n:data[i]);
                    line = String.combine(strings:[line, '   ']);

                    n = numberToAscii(n:data[i]);
                    lineAsText = String.combine(strings:[lineAsText, '   ']);  
                });
                lines->push(value: line + "      " + lineAsText + '\n');
                line = '';
                lineAsText = '';
            };



            iterBytes = endPoint();
            out = String.combine(strings:lines);
            onPageFinish(page:String(from:out));
            
            when(iterBytes >= data.size) send();
        });
    };

};


@:DumpHex = class(
    name : 'DumpHex',
    define:::(this) {

        this.interface = {

            
            
            dump::(buffer => MemoryBuffer.type, onPageFinish) {
                onPageFinish = if(onPageFinish == empty) ::(page) {ConsoleIO.printf(format:page);} else onPageFinish;
                dumphex(data:buffer, onPageFinish:onPageFinish); 
            }
            
        };    
    }
).new();



@:Filesystem = import(module:'Matte.System.Filesystem');
@:buf = Filesystem.readBytes(path:parameters.file);
DumpHex.dump(buffer:buf);

