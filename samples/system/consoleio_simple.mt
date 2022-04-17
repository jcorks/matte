// ConsoleIO gives the ability to interact with standard I/O.
// How is this different than "print()"? Well it's different in a few ways:
// print() isn't guaranteed to print to standard out, but ConsoleIO print*  
// functions are. print() can be implemented however the Matte implementers 
// feel it will be most useful, and on some systems, standard out may not 
// be accessible. ConsoleIO guarantees standard out interaction.
// In addition, ConsoleIO.getln() allows for reading from stdin in the traditional 
// wait-for-input way which is useful for simple commandline tools.
@:Console = import(module:'Matte.System.ConsoleIO');


// This can be used to clear the console in a system-specific way.
Console.clear();

Console.println(message:"Hi. What's your favorite color?");
@result = ::<= {
    // Pauses until input is read.
    @raw = Console.getln();
    
    // most systems will incude the newline. Filter it out.
    return String.replace(string:raw, key:'\n', with:'');
};

// Printf takes a series of tokens and replaces them.
// This option can be more readable that pre-formatting a string.
// The tokens are of the format "$(index)", where index is the 
// index into the args array.
// Note that printf() does not include the newline by default.
Console.printf(format:"Hey! I like the color $(0), too!\n", args:[result]);
