/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the matte project (https://github.com/jcorks/matte)
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


#ifndef H_MATTE__STRING__INCLUDED
#define H_MATTE__STRING__INCLUDED

#include <stdint.h>

///
/// Holds an array of characters, usually meant to convey text.
///
typedef struct matteString_t matteString_t;


/// Creates a new, empty string. 
///
matteString_t * matte_string_create();

/// Creates a new string initialized with the contents of the given C-string 
///
matteString_t * matte_string_create_from_c_str(
    /// The format string.
    const char * fmt, 

    /// The variable args following the format.
    ...
);

/// Creates a new string as a copy of the given string 
///
matteString_t * matte_string_clone(
    /// The string to clone.
    const matteString_t * str
);

/// Creates a new base64-encoded string from a raw byte buffer.
/// This then can be used with matte_string_decode_base64() to retrieve 
/// a raw byte buffer once more.
matteString_t * matte_string_base64_from_bytes(
    /// The raw byte buffer to encode into a base64 string.
    const uint8_t * buffer,
    /// The length of the buffer in bytes.
    uint32_t size
);

/// Returns and allocates a new byte buffer 
/// and its size iff the given string is 
/// correctly formatted as base64.
/// If the string is not correctly encoded,
/// NULL is returned and the size set to 0.
uint8_t * matte_string_base64_to_bytes(
    /// The string encoded into base64.
    const matteString_t *,
    /// The size of the buffer returned.
    uint32_t * size
);




/// Destroys and frees a matte string 
///
void matte_string_destroy(
    /// The string to destroy.
    matteString_t * str
);






/// Sets the contents of the string A to the contents of string B
///
void matte_string_set(
    /// The string to add to.
    matteString_t * A, 

    /// The string to copy from.
    const matteString_t * B
);

/// Resets the contents of the string.
///
void matte_string_clear(
    /// The string to clear.
    matteString_t * str
);


/// Adds the given C printf-formatted string and accompanying arguments 
/// to the given string.
///
void matte_string_concat_printf(
    /// The string to add to.
    matteString_t * str, 

    /// The format of the incoming variable arguments.
    const char * format, 

    /// The variable arguments, indicated by format.
    ...
);


/// Reduces the length of the string in characters.
/// If the new length is larger, no action is taken.
///
void matte_string_truncate(
    /// The string to truncate.
    matteString_t * str,

    /// The new length. Must be smaller than the current 
    /// length to make an effect.
    uint32_t newlen
);

/// Adds the given string B to the end of the given string A.
///
void matte_string_concat(
    /// The string to add to.
    matteString_t * str, 

    /// The string to copy from.
    const matteString_t * src
);


/// Returns a read-only copy of a portion of the given string 
/// from and to denote character indices. The substring is valid until 
/// the next call to this function with the same input string.
const matteString_t * matte_string_get_substr(
    /// The string to read from.
    const matteString_t * str, 

    /// The start index.
    uint32_t from, 

    /// The end index.
    uint32_t to
);



/// Gets a read-only pointer to a c-string representation of the 
/// string.
///
const char * matte_string_get_c_str(
    /// The string to query.
    const matteString_t * str
);

/// Gets the number of characters within the string.
///
uint32_t matte_string_get_length(
    /// The string to query.
    const matteString_t * str
);

/// Gets the character within the string that the given 
/// 0-indexed position. If the position is invalid, 0 is returned.
///
uint32_t matte_string_get_char(
    /// The string to query.
    const matteString_t *, 

    /// The position index within the string.
    uint32_t position
);

/// Sets the character within the string at the given 
/// 0-indexed position. If an invalid position, no action is taken
///
void matte_string_set_char(
    /// The string to modify.
    matteString_t *, 

    /// The position within the string to modify
    uint32_t position, 

    /// The new value within the string.
    uint32_t value
);

/// Adds a character to the end of the string
///
void matte_string_append_char(
    /// The string to append to.
    matteString_t *, 

    /// The value of the character to add to the string.
    uint32_t value
);


/// Gets the byte length of the data representation 
/// of this string. Depending on the context, this could 
/// match the length of the string, or it could be wider.
///
uint32_t matte_string_get_byte_length(
    /// the string to query.
    const matteString_t * str
);

/// Gets the byte data pointer for this strings. Its length is equal to 
/// matte_string_get_byte_length()
///
void * matte_string_get_byte_data(
    /// the string to query.
    const matteString_t * str
);


/// Returns whether substr is found within the given string 
///
int matte_string_test_contains(
    /// The string to search through.
    const matteString_t * str, 

    /// The string to search for.
    const matteString_t * substr
);

/// Returns wither 2 strings are equivalent 
///
int matte_string_test_eq(
    /// The string to test equivalence.
    const matteString_t * str, 

    /// The other string to test.
    const matteString_t * other
);

/// Compares the 2 strings in a sort-ready fashion:
/// Returns < 0 if a alphabetically comes before b
/// Returns > 0 if a alphabetically comes after b
/// Returns = 0 if a and b are equivalent
int matte_string_compare(
    /// The first string to compare.
    const matteString_t * a, 

    /// The second string to compare.
    const matteString_t * b
);



#endif

