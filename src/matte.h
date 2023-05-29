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
#ifndef H_MATTE__CONTEXT__INCLUDED
#define H_MATTE__CONTEXT__INCLUDED


/// The Matte instance. Owns all the major components 
/// required to run and manage Matte. Also contains 
/// higher-level functions that provide most 
/// basic embedding features.
///
typedef struct matte_t matte_t;
typedef struct matteVM_t matteVM_t;
typedef struct matteSyntaxGraph_t matteSyntaxGraph_t;

#include "matte_store.h"




/// Creates a new Matte instance
///
matte_t * matte_create();


/// Sets the functions to use for all memory allocations 
/// and deallocations. 
/// For correct use, call this before ANY Matte calls.
///
void matte_set_allocator(
    /// Optional allocator. If specified, this will 
    /// be used instead of matte_allocate() for all memory allocations.
    /// When NULL, the default allocator (C malloc) will be used.
    void * (*allocator)  (uint64_t),
    
    // Optional deallocator. If specified, this will 
    // be used instead of matte_deallocate() for all memory allocations
    // When NULL, the default deallocator (C free) will be used.
    void   (*deallocator)(void*)
);


/// Destroys a Matte instance and all owned components.
///
void matte_destroy(
    /// The array to destroy
    matte_t *
);





/// Connects default IO wrappers for many of 
/// Matte's VM features, making it much more 
/// convenient to set up the most common 
/// runtime options. This sets a default handler for 
/// the VM print function as well as unhandled errors.
///
void matte_set_io(
    /// The instance to set IO.
    matte_t *,
    
    /// The function to handle needed input 
    /// from the user. If no input is ready, NULL can be 
    /// returned, and the input function will be called again.
    /// If NULL, stdin is used.
    ///
    char * (*input) (matte_t *),       
    
    /// The function to handle needed output to inform 
    /// the user.
    /// If NULL, stdout is used.    
    ///
    void   (*output)(matte_t *, const char *),
    
    /// The function to clear output from Matte output.
    /// If NULL, stdout is cleared using ANSI terminal characters.
    ///
    void   (*clear)(matte_t *)
);  



/// When called, enables debugging features with default handlers
/// using the IO functions within matte_set_io.
///
/// After calling this function, a debugger will be available using IO features 
/// creating an environment where code is viewable, commands may be issued,
/// expressions may be run in-scope, and more. It provides a default 
/// basis for a standard debugger.
///
void matte_debugging_enable(
    /// The instance to enable IO for
    matte_t *
);


/// Adds an external C function for use within the VM.
/// This follows the semantics of matte_vm_set_external_function()
/// but is more convenient.
///
/// In the script context: calling getExternalFunction() with the string identifier 
/// given will return a function value that, when called, calls this C function.
///
/// The args are given in the order that you specific in the 
/// argNames array. The number of values within the C function 
/// will always match the number of argNames for the function.
///
/// If the calling context does not bind an argument, the slot within the 
/// called C function will be an empty value.
///
/// Optionally, a number of C Strings (UTF8 encoded) may be 
/// specified as argument names. If undesired, the last argument should be 
/// NULL. The number of names specified determines how many inputs 
/// the function has, and consequently, how many values are 
/// to be expected in the args array when the C function is valled.
///
void matte_add_external_function(
    /// The instance to set the external function within.
    matte_t *, 

    /// The identifier of the function.
    const char * name, 

    /// The function to be called when run in the VM.
    /// The VM instance is passed in along with the function value itself, the arguments 
    /// (which are always of the size of the number of bound input names), and any user 
    /// data.
    matteValue_t (*)(matteVM_t *, matteValue_t fn, const matteValue_t * args, void * userData),

    /// User data. Passed along with the called C function.
    void *,
    
    /// A NULL-terminated series of C strings (UTF8 encoded) that 
    /// signify the arguments to the function.
    ... 
);


/// Follows the semantics of matte_vm_call(), but is easier to 
/// use. 
/// Functions are called immediately and only return once computation is finished.
/// This includes any sub-calls to matte_call / matte_vm_call. The return value 
/// of the function is returned. If the call fails to complete, such as 
/// with an error, the return value is empty.
/// Additional arguments may be provided to bind parameters to their 
/// respective names. A C String (UTF8 encoded) followed by a matteValue_t shall be provided if 
/// desired. Regardless, the final argument must be NULL
///
matteValue_t matte_call(
    /// The Matte instance to apply the function call.
    matte_t *, 

    /// The function to call.
    matteValue_t func, 

    /// A NULL-terminated series of C-String - matteValue_t pairs, representing 
    /// the inputs to the function call by-name.
    ... 
);


/// Convenience function that runs the given source.
/// Internally, it is given a unique name, compiled, and
/// runs the new fileID immediately. This will also provide 
/// a default handler for compilation error, piped through 
/// the matte output IO function. If an unhandled error occurs,
/// empty will be returned.
///
matteValue_t matte_run_source(
    /// Instance to run.
    matte_t *, 

    /// C-String representation of the source.    
    const char * source
);


/// Convenience function that compiles the source into bytecode.
/// A default handler for the compilation is invoked if 
/// an error occurs. Errors will result in a NULL 
/// return value. matte_deallocate should be called on 
/// the resultant buffer to release it when done.
///
uint8_t * matte_compile_source(
    /// Instance to compile with.
    matte_t *, 
    
    /// Output length of bytecode.
    uint32_t * bytecodeSize, 

    /// The C-String source to compile.    
    const char * source
);


/// Convenience function that registers a module by-name with 
/// the given bytecode. The module is not run, but pre-loaded so that, 
/// when imported, the bytecode is run at that time.
/// 
void matte_add_module(
    /// The instance to add the module to.
    matte_t *, 
    
    /// The identifier of the module. This is the 
    /// name that will be used to import.
    const char * name, 
    
    /// The bytecode to add as the module.
    const uint8_t * bytecode, 
    
    /// The length of the buffer containing 
    /// the bytecode.
    uint32_t bytecodeSize
);



/// Sets the import function for the instance.
/// By default, Matte calls to import only look for 
/// the built-in modules and any added modules.
/// However, it's typically more useful to 
/// work with other sources of bytecode and 
/// Matte source.
void matte_set_importer(
    /// The matte instance
    matte_t *,
    
    /// The function to call when the VM has requested 
    /// an import. The function should return the fileid 
    /// represented by the Matte bytecode to run.
    /// For a custom importer, it will be necessary to 
    /// manually convert source / bytecode to a fileID by 
    /// hand using the matte_vm_* family of functions.
    ///
    /// When NULL, a default file-based importer will be used.
    uint32_t(*)(matte_t *, const char * name, void * userData),
    
    // Optional user data for the importer function
    void * userData
);


/// Loads a package file and preloads all its 
/// sources. Any trailing packages are also loaded.
/// If any of the embedded packages fail to be 
/// correctly read, a value of type empty
/// and an error within the VM is raised. 
/// matte_load_package will try to read and load 
/// as much as it can and will stop on error.
/// The main.mt is preloaded as the name of the package, while 
/// all other sources are labeled as [package name].[source name]
/// when preloaded.
/// An object containing the parsed JSON describing 
/// the package.
/// Package JSON objects have the following attributes:
/*
    {
        'name':         String,
        'author':       String,
        'maintainer':   String,
        'description':  String,

        'version', Object,
    
        'depends',      Object, 
        'sources',      Object
    }
*/
///
/// The value is owned and reserved by the VM,
/// so it shoud not be recycled.
matteValue_t matte_load_package(
    /// The instance to load the package into.
    matte_t *,
    /// A buffer containing the bytes the consist of the package.
    const uint8_t * packageBytes,
    /// The number of bytes in the buffer.
    uint32_t packageByteLength
);


/// Returns the parsed JSON package object.
/// If no such one exists, the empty value is returned.
///
/// The value is owned and reserved by the VM,
/// so it shoud not be recycled.
matteValue_t matte_get_package_info(
    /// The instance to retrieve package info from
    matte_t *,
    /// The name of the package.
    const char * packageName
);
    
    
/// Creates a new array of matteString_t *
/// containing the dependency package names.
/// If no such package has been loaded, NULL is returned.
/// In the case an array is returned, the caller is 
/// responsible for cleaning up the array and strings.
matteArray_t * matte_get_package_dependencies(
    /// The instance to query
    matte_t *,
    
    /// The name of the package
    const char * packageName
);    

/// Checks to see whether the package has all its 
/// dependencies met. This checks all loaded packages 
/// and their versions to see if it is safe to 
/// run. If it is not, an error is thrown 
/// and 0 is returned.
/// While it is not necessary to call this on any package,
/// it is helpful as a preflight check to make sure the 
/// package will run properly.
int matte_check_package(
    /// The instance to check.
    matte_t *,
    /// The package to check
    const char * packageName
);


/// Convenience function that runs the given bytecode.
/// Internally, it is given a unique name and
/// runs the new fileID immediately. If an unhandled error 
/// occurs, empty is returned.
///
/// The value is owned and reserved by the VM,
/// so it shoud not be recycled.
matteValue_t matte_run_bytecode(
    /// The instance to run the bytecode on.
    matte_t *, 
    
    /// The bytecode to run.
    const uint8_t * bytecode, 
    
    /// the length of the buffer containing the bytecode.
    uint32_t bytecodeSize
);



/// same as matte_run_bytecode, except with a parameters object.
///
/// The value is owned and reserved by the VM,
/// so it shoud not be recycled.
matteValue_t matte_run_bytecode_with_parameters(
    /// The instance
    matte_t *, 

    /// The bytecode to run.
    const uint8_t * bytecode, 
    
    /// the length of the buffer containing the bytecode.
    uint32_t bytecodeSize,
    
    /// The value to be the special "parameters" value.
    matteValue_t params
);


/// Same as matte_run_source, except with a parameters object
///
/// The value is owned and reserved by the VM,
/// so it shoud not be recycled.
matteValue_t matte_run_source_with_parameters(
    /// The instance to run with.
    matte_t *, 
    
    /// The C-String containing Matte source.
    const char * source, 
    
    /// The value to be the special "parameters" value.
    matteValue_t params
);


/// Nicely prints a value. The string persists
/// until the next call to the function, where it is 
/// freed and a new string is returned. The string 
/// is owned by the instance.
const char * matte_introspect_value(
    /// The instance to use to introspect.
    matte_t *, 
    
    /// The value to introspect.
    matteValue_t
);


/// Gets generic data to be assigned to the matte instance
/// The default value is NULL.
///
void * matte_get_user_data(
    /// The instance to retrieve the data from.
    matte_t * m
);

/// Sets generic data to be assigned to the matte instance
///
void matte_set_user_data(
    /// The instance to set the data to.
    matte_t * m, 
    
    /// The target to serve as the generic data.
    void * d
);


/// Gets the VM instance from the matte instance.
/// The VM is owned by the Matte instance.
///
matteVM_t * matte_get_vm(
    /// The instance to retrieve the VM from.
    matte_t *
);


/// Gets the syntax graph from the matte instance.
///
matteSyntaxGraph_t * matte_get_syntax_graph(
    /// The instance to retrieve the graph from.
    matte_t *
);

/// Allocates a buffer of size bytes
/// All bytes are set to 0. Upon error
/// NULL is returned. A request to allocate 
/// 0 bytes always results in NULL.
/// The allocator functions are meant for 
/// use within matte to obey the allocator rules.
/// User use is discouraged.
///
void * matte_allocate(
    /// Number of bytes to allocate.
    uint64_t size
);

/// Deallocates a buffer previously returned 
/// by matte_allocate(). if NULL is passed,
/// no action is taken. UNDEFINED BEHAVIOR MAY OCCUR 
/// if a pointer is passed that wasnt returned by 
/// matte_allocate(). 
/// The allocator functions are meant for 
/// use within matte to obey the allocator rules.
/// User use is discouraged.
///
void matte_deallocate(void *);

#endif
