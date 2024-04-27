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
// ConsoleIO gives the ability to interact with standard I/O.
// How is this different than "print()"? Well it's different in a few ways:
// print() isn't guaranteed to print to standard out, but ConsoleIO print*  
// functions are. print() can be implemented however the Matte implementers 
// feel it will be most useful, and on some systems, standard out may not 
// be accessible. ConsoleIO guarantees standard out interaction.
// In addition, ConsoleIO.getln() allows for reading from stdin in the traditional 
// wait-for-input way which is useful for simple commandline tools.
@:Console = import(:'Matte.System.ConsoleIO');


// This can be used to clear the console in a system-specific way.
Console.clear();

Console.println(:"Hi. What's your favorite color?");
@result = ::<= {
    // Pauses until input is read.
    @raw = Console.getln();
    
    // most systems will incude the newline. Filter it out.
    return raw->replace(key:'\n', with:'');
}

// Printf takes a series of tokens and replaces them.
// This option can be more readable that pre-formatting a string.
// The tokens are of the format "$(index)", where index is the 
// index into the args array.
// Note that printf() does not include the newline by default.
Console.printf(format:"Hey! I like the color $(0), too!\n", args:[result]);
