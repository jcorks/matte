# Matte: A simple language with minimal syntax

## Current status: PRE RELEASE


 - For information on current status: https://jcorks.github.io/matte/
 - For a language overview: https://jcorks.github.io/matte/doc-quick.html



Matte is a small, simple, embeddable language geared for maintainable development.
Written in C itself, it features a C-like syntax with a set of key differences 
designed to approach problems in a more maintainable way.


The language can either be used by compiling a set of dependency-free, C sources 
and linking C functions to Matte functions, or it can be used with a standalone 
interpreter. The interpreter extends the language with common OS features, such 
as I/O.


In the most broad sense, think of Matte like embeddable Javascript put through a filter 
with some other features added and removed, and required named function parameters.

### Main Language Features:

 - Small and easily embeddable with no external dependencies required
 - Both C / C++ compatible code
 - Dynamic, garbage collected value system
 - Optional gradual typing
 - Closures with lexical scoping
 - Simple error catching 
 - Simple module management 
 - Built-in modules for classes, strings, JSON, and more 
 - Optional OS feature extension modules for asynchronous computation, socket I/O, and more 
 - Dump to/from portable VM bytecode
 - Built-in optional runtime debugging features 
 - Easily extensible 


Here's what it looks like:
```
@:sayHi = ::(greeting => String) {
    print(message:greeting)
}

sayHi(greeting: 'Hello, world!')

```


Here's a more in-depth look:
```

// Combines inputs.
// Requires the parameter to be an Object
@:combine = ::(items => Object) { 
    // Reduce set
    return items->reduce(to:::(previous, value) {
        // Immediately exit if previous is not defined.
        when (previous == empty) value;
        
        // Else append
        return previous + value;
    })
}
    
print(message:combine(items:['Come ', 'try ', 'it ', 'for ', 'yourself!']))




// Combines inputs as one expression
@:combineAsOneExpression = ::(items) <- 
    // Use inline function to return a single expression
    items->reduce(to:::(previous, value) <- 
        // Use another inline function to determine how to combine
        if (previous == empty) value else previous + value
    )
; 

print(message:combineAsOneExpression(items:['See ', 'what ', 'you ', 'can ', 'make!']))




// Combines inputs using a forloop.
// Also does some basic case accounting
@:combineUsingAForloop = ::(strings => Object) {
    // If no strings, just return the empty string
    when (strings->keycount == 0) ''
    
    // No need to combine if only one string!
    when (strings->keycount == 1) strings[0]
    
    // initial string is the first in the set
    @output = strings[0]
    
    // Since we have the first value, take a subset of the 
    // list, skipping the first value. Then, 
    // iterate over the list.
    foreach(strings->subset(
        from:1, 
        to:strings->keycount-1
    )) ::(index, string) {
            output = output + string;
        }
    )
    
    return output
}

print(message:combineUsingAForloop(strings:['You ', 'never ', 'know ', 'what ', "you'll ", 'find!']))

```


Curious? Check out the quick guide to get a feel for the concepts.

## Using this repository

The sources provided in this repository provide the base VM (virtual machine) and runtime 
to embed a Matte interpreter into any project. Also included is a standalone interpreter 
that can be run and includes system-oriented extensions to allow Matte to be used as 
a useful system tool.

For convenience, Matte uses [GNU make](https://www.gnu.org/software/make/) to build 
parts of the project, but it is not necessary to use: it is totally possible to 
build the project without it by simply looking at the commands in the referenced makefiles and running them yourself. Matte's 
construction is simple by design, and `make` is simply used as a batch program to facilitate construction generically and easily on 
Unix-like systems and on the [MSYS2](https://www.msys2.org/) system on Windows.


### Preparing the VM (for standalone or embedded use)

The Matte runtime implemented here uses a ROM file to embed its compiled 
base modules, making the ROM required for any usage of Matte.
The ROM is made using a small sub-program that just compiles 
the base module sources and packs the bytecode binary into a file.

To prepare the ROM file:

 - Go to src/rom/
 - Decide if you want your ROM to include system extensions
 - Run `make` to produce the ROM-making program. If using system extensions, you can either run make with no options or with the `with-extensions` recipe. If not using extensions, use the `no-extensions` recipe. If not using `make`, open the `makefile` in a text editor and run the commands based on which version you would like with your C/C++ compiler.
 - Run the output ROM-making program `makerom`.

The output program (based on makerom.c), produces a MATTE_ROM file, which packs
the rom in a compilable C source file in the src/ directory.



### Embedding Matte 

One of the most useful aspects of Matte is that, due to its simplicity, it 
can be easily dropped in any projects as-is with **no additional modification or dependencies**:

 - Make sure the ROM is made with the specifications you like (i.e. with or without system extensions)
 - Copy over all the contents of src/ to some directory in your project 
 - Set your build system to compile:
   - all .c files in the src/ directory
   - all the .c files src/rom/core
   - any system extension .c files if needed for your system. (i.e. on Windows, src/rom/system/winapi/)
  
 

See samples/embedding/ for practical examples on how to embed Matte.



### Building the standalone interpreter

The standalone tool is useful for prototyping Matte code, debugging Matte code, and having a simple REPL (author's note: I use it as a desktop calculator!)

To build it:

 - Go to cli/
 - Run `make` with the makefile relevant to your system to produce `matte`, the standalone interpreter. For example, on a Unix-like system, one can run `make -f makefile_unix` to produce the `matte` binary.

### Using the Matte interpreter

Once produced, the Matte interpreter can be used in a few different modes.


#### REPL mode

Running `matte` with no options runs the interpreter in a [REPL](https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop) mode. 
Any Matte expression can be run and its output printed. This includes dash functions, so 
essentially the entire language, although inconvenient due to the single-line input mode, 
is possible to use in this mode.

Additionally, a persistent `store` object exists and is defined. Storage of expression 
results is possible by running expressions that modify the `store` object, such as 
`store.myVariable = 2+3`, or `store.func = ::<- 'Hello, world!'`

The result of the expression output to the interpreter will result in 
the expression being computed as a string through the `Matte.Core.Introspect` module function.

#### Running Matte Source Files 

The most general way of using the interpreter is to feed it a source file.
Doing so can be done by just passing the file name as an argument to the 
program. Parameters can be passed to the source file by appending parameters 
in the format `name:value`. For example, on a Unix-like system one could run a script where a parameter 
`input` could be set to 3 as:

`./matte MySource.mt input:3`

This command can also be used to run bytecode blobs, as discussed in the next section.


#### Compiling Matte Sources 

It is possible to use the standalone interpreter to compile Matte source into portable bytecode blobs.
*This bytecode is understood by **both** the standalone interpreter and embedded 
instances of the VM*, making it very useful as a solution where operation is desired without the original source.

This is invoked using the "compile" command:

`./matte compile MySource.mt MySource_Output.o`


#### Debugging Matte Sources 

The Matte interpreter includes tools to debug Matte sources by 
providing step-wise debugging interface, similar to [GDB](https://en.wikipedia.org/wiki/GNU_Debugger), providing 
interfacing with the `breakpoint()` function, stepping through instructions, and in-scope
expression printing. 

Debugging is started through the `debug` tool:

`./matte debug MySource.mt`

Note that if the `breakpoint()` function is not called in the target code or included modules, execution will not 
be altered.

The currently-understood commands are as follows:
 - `p` / `print` : prints an expression, which can include a variable name in scope.
 - `s` / `step`  : evaluates the next instruction in the VM, which may "step into" a function call.
 - `n` / `next`  : evaluates the next instruction in the VM, automatically running any instruction that would generate further function scopes. This essentially lets you step to the next instruction in the same scope or higher.
 - `bt` / `backtrace` : prints the entire callstack.

(*These same commands and behaviors are generically modeled and possible to be implemented by any program using embedded Matte. See the functions in src/matte.h and the samples/embedding/debug/ example*)

#### Samples

Samples exist demonstrating use of Matte and the standalone interpreter.
They are located in samples/matte/ and can be run as any source file would.
For example, on Unix-like systems, one example implements ["Rule 90" elementary cellular automaton](https://en.wikipedia.org/wiki/Rule_90) and prints it to standard out:

`./matte ../samples/matte/wolfram.mt`





