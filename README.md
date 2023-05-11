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




