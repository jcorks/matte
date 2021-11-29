@MemoryBuffer = import('Matte.System.MemoryBuffer');
@ConsoleIO    = import('Matte.System.ConsoleIO');
@MatteString  = import('Matte.Core.String');
@class        = import('Matte.Core.Class');


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
<@> numberToHex::(n => Number) {
    return hextable[n / 16] + // number lookups are always floored.
           hextable[n % 16];
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



// config
@ BYTES_PER_LINE = 8;






<@> numberToAscii::(n => Number) {
    <@> res = asciitable[n];
    when(res == empty) '__';
    return res + ' ';
};

<@>line = MatteString.new();
<@>lineAsText = MatteString.new();




<@> dumphex ::(data => MemoryBuffer.type){
    line.length = BYTES_PER_LINE*2+BYTES_PER_LINE;
    for([0, BYTES_PER_LINE], ::(i) {
        line[i+2] = ' ';
    });

    lineAsText.length = BYTES_PER_LINE*2;

    @iter = 0;
    @lineIter = 0;
    @lineAsTextIter = 0;
    @charAt = introspect.charAt;
    @out = MatteString.new();
    for([0, data.size], ::(i) {
        if (i%BYTES_PER_LINE == 0) ::<={
            out += line + "      " + lineAsText + '\n';
            iter = 0;
            lineAsTextIter = 0;
            lineIter = 0;
        };

        @n = numberToHex(data[i]);
        line[lineIter] = charAt(n, 0); lineIter+= 1;
        line[lineIter] = charAt(n, 1); lineIter+= 1;
        lineIter+= 1;

        n = numberToAscii(data[i]);
        lineAsText[lineAsTextIter] = charAt(n, 0); lineAsTextIter += 1;            
        lineAsText[lineAsTextIter] = charAt(n, 1); lineAsTextIter += 1;            


        iter += 1;
    });
    
    if (iter != 0) ::<= {
        for([iter, BYTES_PER_LINE], ::{
            line[lineIter] = ' '; lineIter+= 1;
            line[lineIter] = ' '; lineIter+= 1;
            lineIter+= 1;

            lineAsText[lineAsTextIter] = ' '; lineAsTextIter+= 1;
            lineAsText[lineAsTextIter] = ' '; lineAsTextIter+= 1;
        });
        out += line + "      " + lineAsText + '\n';
    };
    return out;
};


return class({
    name : 'DumpHex',
    define::(this) {

        this.interface({
            bytesPerLine : {
                get :: {
                    return BYTES_PER_LINE;
                },
                
                set ::(v) {
                    BYTES_PER_LINE = v;
                }
            },
            
            
            'print' ::(m => MemoryBuffer.type) {
                ConsoleIO.println(String(dumphex(m)));                      
            },
            
            toString ::(m => MemoryBuffer.type) => String {
                return String(dumphex(m));
            }
        });    
    }
}).new();

