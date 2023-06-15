/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
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


#ifndef H_MATTE__ARRAY__INCLUDED
#define H_MATTE__ARRAY__INCLUDED


#include <stdint.h>





/// Dynamically resizing container
///
/// Standard containers in many languages, this will hold shallow data
/// and resize itself as need be. 
///
typedef struct matteArray_t matteArray_t;



/// Returns a new, empty array 
///
/// sizeofType refers to the size of the elements that the array will hold 
/// the most convenient way to do this is to use "sizeof()"
matteArray_t * matte_array_create(
    /// Size of the object that the array should hold.
    uint32_t sizeofType
);

/// Destroys the container and buffer that it manages.
///
void matte_array_destroy(
    /// The array to destroy.
    matteArray_t * array
);


/// Returns an empty, read-only array. 
///
const matteArray_t * matte_array_empty();

/// Clones an entire array, returning a new array instance.
///
matteArray_t * matte_array_clone(
    /// The source array to clone.
    const matteArray_t * array
);



/// Adds an additional element to the array.
///
/// The value given should be an addressable value. For example a 
/// value "12" will not be able to be pushed, but if 
/// setting a variable ahead of time and adding 
/// "i = 12; matte_array_push(array, i);" would work.
/// Example:
///
///     int i = 0;
///     matteArray_t * arr = matte_array_create(sizeof(int));
///     matte_array_push(arr, i);
///
#define matte_array_push(__A__,__VAL__) (matte_array_push_n(__A__, &__VAL__, 1))

/// Gets the value at the given index.
/// Example:
///     
///     int i = 42;
///     matteArray_t * arr = matte_array_create(sizeof(int));
///     matte_array_push(arr, i);
///     printf("%d", matte_array_at(arr, int, 0));
///
/// The above example would print 42.
///
#define matte_array_at(__A__,__T__,__I__) (((__T__*)matte_array_get_data(__A__))[__I__])

/// Adds a contiguous set of elements to the array 
///
void matte_array_push_n(
    /// The array to modify.
    matteArray_t * array, 

    /// An input array of data. The element array is assumed to be 
    /// aligned to the size of the matte array's object size.
    const void * element, 

    /// The number of items within the element array.
    uint32_t count
);


/// Removes a specific member of the array
///
void matte_array_remove(
    /// The array to modify.
    matteArray_t * array, 

    /// The index to remove.
    uint32_t index
);


/// Removes count members of the array starting at the given index.
///
void matte_array_remove_n(
    /// The array to modify.
    matteArray_t * array, 

    /// The index to start to remove.
    uint32_t index,
    
    /// The number of members to remove.
    uint32_t count
);



/// Compares 2 elements within an array.
/// Each points to an element within an array; this is used 
/// for matte_array_lower_bound(). Its expected that 
/// the return value returns whether A is "less" than B.
typedef int (*matte_array_comparator) (
    /// Pointer to array element A.
    const void * a, 

    /// Pointer to array element B.
    const void * b
);

/// Returns the index that this element should be inserted into 
/// given that the rest of the array is sorted. If the element is 
/// greater than the rest of the array's elements, the index 
/// after the array is returned.
///
uint32_t matte_array_lower_bound(
    /// The array to query.
    const matteArray_t * array, 

    /// The element in question
    ///    
    const void * element, 

    /// Returns whether the value that a points to is "less" than 
    /// the value that b points to.
    ///
    matte_array_comparator less
);

/// Inserts the given value at the given position. If the position 
/// is above or equal to the size of the array, the new element is 
/// placed at the end.
///
#define matte_array_insert(__A__,__I__,__V__) (matte_array_insert_n(__A__, __I__, &(__V__), 1))



/// Inserts the given number of elements at the given index.
///
void matte_array_insert_n(
    /// The array to insert data into.
    matteArray_t * array, 

    /// The index to insert at.
    uint32_t index, 

    /// The source data to insert into the array.
    void * element, 

    /// The number of elements to insert.
    uint32_t count
);





/// Sets the size of the array
/// If the array were to be larger than it could account for,
/// this invokes an internal resize
///
void matte_array_set_size(
    /// The array to modify.
    matteArray_t * array,

    /// The new size. 
    uint32_t size
);

struct matteArray_t {
    uint32_t allocSize;
    uint32_t size;
    uint32_t sizeofType;
    uint32_t padding0;
    uint8_t * data;
};


/// Returns the size of the array
///
#define matte_array_get_size(__A__) ((__A__)->size)

/// Returns the size of each element in bytes.
///
#define matte_array_get_type_size(__A__) ((__A__)->sizeofType)

/// Gets a pointer to the raw data of the array
/// This pointer is guaranteed to be a contiguous memory block of the 
/// current state of the array. It is editable.
///
#define matte_array_get_data(__A__) ((void*)(__A__)->data)

#define matte_array_shrink_by_one(__A__) ((__A__)->size ? (__A__)->size-- : 0)

/// Creates a temporary array whos data is managed for you.
/// Temporary arrays are "read only" and should to be modified.
matteArray_t matte_array_temporary_from_static_array(
    /// The data to populate the temporary array for you.
    void * array, 

    /// Size of the object within the input array.
    uint32_t sizeofType, 

    /// The object count that the array holds.
    uint32_t length
);

/// Convenience call for matte_array_temperator_from_static_array() 
/// The first argument is the array pointer
/// The second argument is the type of each member within the array 
/// The third argument is the number of members in the array
///
#define MATTE_ARRAY_CAST(__D__,__T__,__L__) (matte_array_temporary_from_static_array(__D__, sizeof(__T__), __L__))


#endif
