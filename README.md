Matte: A simple language with minimal syntax
============================================

Current status: PRE RELEASE
---------------------------


* For information on current status: https://jcorks.github.io/matte/
* For a language overview: https://jcorks.github.io/matte/doc-quick.html



Matte is a small, simple, embeddable language geared for maintainable development.
Written in C itself, it features a C-like syntax with a set of key differences 
designed to approach problems in a more maintainable way.


The language can either be used by compiling a set of dependency-free, C sources 
and linking C functions to Matte functions, or it can be used with a standalone 
interpreter. The interpreter extends the language with common OS features, such 
as I/O.


In the most broad sense, think of Matte like embeddable Javascript put through a filter 
with some other features added and removed.

Main Language Features:


* Small and easily embeddable with no external dependencies required
* Both C / C++ compatible code
* Dynamic, garbage collected value system
* Optional gradual typing
* Closures with lexical scoping
* Simple error catching 
* Simple module management 
* Built-in modules for classes, strings, JSON, and more 
* Optional OS feature extension modules for asynchronous computation, socket I/O, and more 
* Dump to/from portable VM bytecode
* Built-in optional runtime debugging features 
* Easily extensible 


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



