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
@MemoryBuffer = import(module:'Matte.Core.MemoryBuffer');
@ConsoleIO    = import(module:'Matte.System.ConsoleIO');
@class        = import(module:'Matte.Core.Class');


when(parameters == empty || parameters.file == empty) ::<={
    print(message:'DumpHex usage:');
    print(message:'');
    print(message:'matte DumpHex.mt file:[filename]');

}


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
}

// assumes 0-255
@: numberToHex::(n => Number) {
    return {first :hextable[n / 16], // number lookups are always floored.
     second:hextable[n % 16]}
}

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
}









@: numberToAscii::(n => Number) {
    @: res = asciitable[n];
    when(res == empty) {
        first: '_',
        second: '_'
    }
    return {
        first: res,
        second: ' '
    }
}

@line;
@lineAsText = '';




@: dumphex ::(data => MemoryBuffer.type, onLineFinish => Function){
    line = '';
    for(0, BYTES_PER_LINE*2+BYTES_PER_LINE)::(i) {
        line = line + ' ';
    }
    lineAsText = '';
    for(0, BYTES_PER_LINE*2)::(i) {
        lineAsText = lineAsText + ' ';
    }



    @iterBytes = 0;
    
    @:endPoint :: {
        when(iterBytes+BYTES_PER_PAGE >= data.size) data.size;
        return iterBytes+BYTES_PER_PAGE;
    }


    @lines = [];
    @linePoints = [];
    @lineAsTextPoints = [];
    @iter = 0;

    @:flush ::{
        lines->push(value: 
            String.combine(strings:[                        
                String.combine(strings:linePoints),
                "      ",
                String.combine(strings:lineAsTextPoints),
                '\n'
            ])
        );;
        iter = 0;
        linePoints->setSize(size:0);
        lineAsTextPoints->setSize(size:0);    
    }

    {:::} {
        forever ::{
            @lineIter = 0;
            @lineAsTextIter = 0;
            @out = '';
        
            for(iterBytes, endPoint()) ::(i) {
                if (i%BYTES_PER_LINE == 0) ::<={
                    flush();
                }

                @n = numberToHex(n:data[i]);
                linePoints->push(value:n.first);
                linePoints->push(value:n.second);
                linePoints->push(value:' ');

                n = numberToAscii(n:data[i]);
                lineAsTextPoints->push(value:n.first);
                lineAsTextPoints->push(value:n.second);

                iter += 1;
            }
            
            if (iter%BYTES_PER_LINE) ::<= {
                flush();
            }
            


            iterBytes = endPoint();
            foreach(lines) ::(k, v) {
                onLineFinish(page:v);
            }
            lines->setSize(size:0);
            
            when(iterBytes >= data.size) send();
        }
    }

}


@:DumpHex = class(
    name : 'DumpHex',
    define:::(this) {

        this.interface = {

            
            
            dump::(buffer => MemoryBuffer.type, onLineFinish) {
                onLineFinish = if(onLineFinish == empty) ::(page) {ConsoleIO.printf(format:page);} else onLineFinish;
                dumphex(data:buffer, onLineFinish:onLineFinish); 
            }
            
        }    
    }
).new();



@:Filesystem = import(module:'Matte.System.Filesystem');
@:buf = Filesystem.readBytes(path:parameters.file);
DumpHex.dump(buffer:buf);

