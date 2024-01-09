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
#ifndef H_MATTE__OPCODE__INCLUDED
#define H_MATTE__OPCODE__INCLUDED

typedef enum {
    // no operation
    MATTE_OPCODE_NOP,
    
    
    // push referrable onto value stack of current frame.
    MATTE_OPCODE_PRF,
    // push empty 
    MATTE_OPCODE_NEM,
    // push new number,
    MATTE_OPCODE_NNM,
    // push new boolean,
    MATTE_OPCODE_NBL,
    // push new string
    MATTE_OPCODE_NST,
    // push new empty object
    MATTE_OPCODE_NOB,
    // push new function from a compiled stub
    MATTE_OPCODE_NFN,

    // costructor object assignment
    // with an object, key, and value,
    // assigns bypassing normal assignment 
    // checks. The object is left on the stack 
    // afterwards.
    MATTE_OPCODE_CAS,
    
    // costructor array assignment
    // with an array and value,
    // assigns bypassing normal assignment 
    // checks. The object is left on the stack 
    // afterwards.
    MATTE_OPCODE_CAA,    
    

    // push a new callstack from the given function object.
    MATTE_OPCODE_CAL,
    // assign value to a referrable 
    MATTE_OPCODE_ARF,
    // assign object member
    MATTE_OPCODE_OSN,
    // lookup member
    MATTE_OPCODE_OLK,

    // general purpose operator code.
    MATTE_OPCODE_OPR,
    
    // call c / built-in function
    MATTE_OPCODE_EXT,

    // pop values from the stack
    MATTE_OPCODE_POP,
    // copys top value on the stack.
    MATTE_OPCODE_CPY,

    // return;
    MATTE_OPCODE_RET,
    
    // skips PC forward if conditional is false. used for when conditional return
    MATTE_OPCODE_SKP ,
    // skips PC forward always.
    MATTE_OPCODE_ASP,
    // pushes a named referrable at runtime.
    // performance heavy; is not used except for 
    // special compilation.
    MATTE_OPCODE_PNR,
    
    // Pushes the result of an listen expression.
    // listen pops 2 functions off the stack and pushes 
    // the result of the listen operation.
    MATTE_OPCODE_LST,
    
    // pushes a abuilt-in type object 
    // 
    // 0 -> empty 
    // 1 -> boolean 
    // 2 -> number 
    // 3 -> string 
    // 4 -> object
    // 5 -> type (type of the type object) 
    // 6 -> any (not physically in code, just for the vm)
    MATTE_OPCODE_PTO,
    
    
    // indicates to the VM that the next NFN is 
    // a strict function.
    MATTE_OPCODE_SFS,

    // Indicates a query
    MATTE_OPCODE_QRY,


    // Shhort circuit: &&
    // Peek the top. If false, skip by given count
    MATTE_OPCODE_SCA,

    // Shhort circuit: OR
    // Peek the top. If true, skip by given count
    MATTE_OPCODE_SCO,

    // spread operator: array.
    // pops the top of the stack and push all values of the object.
    MATTE_OPCODE_SPA,

    // spread operator: object 
    // pops the top of the stack and pushes all keys and values of the object.
    MATTE_OPCODE_SPO,
    
    // Object: Assign Set 
    // Allows for many assignments at once to an object using the 
    // dot operator. The syntax is operand.{static object};
    // The first operand is pushed to the top of the stack.
    MATTE_OPCODE_OAS,
    
    // performs a loop, taking the top 3 variables on the stack to define a 
    // loop: from value[top-3] to value[top-2], run function value[top-1]
    MATTE_OPCODE_LOP,

    // performs an infinite loop, calling the value[top-1] function indefinitely.
    MATTE_OPCODE_FVR,

    // performs a foreach loop on an object (value[top-1])
    MATTE_OPCODE_FCH,
    
    // VarArg call, calls with an object pushed as a single argument to be 
    // distributed across the matched argument names in the calling function
    MATTE_OPCODE_CLV,
    
    MATTE_OPCODE_ERROR = -1

} matteOpcode_t;


typedef enum {
    MATTE_OPERATOR_ADD, // + 2 operands
    MATTE_OPERATOR_SUB, // - 2 operands
    MATTE_OPERATOR_DIV, // / 2 operands
    MATTE_OPERATOR_MULT, // * 2 operands
    MATTE_OPERATOR_NOT, // ! 1 operand
    MATTE_OPERATOR_BITWISE_OR, // | 2 operands
    MATTE_OPERATOR_OR, // || 2 operands
    MATTE_OPERATOR_BITWISE_AND, // & 2 operands
    MATTE_OPERATOR_AND, // && 2 operands
    MATTE_OPERATOR_SHIFT_LEFT, // << 2 operands
    MATTE_OPERATOR_SHIFT_RIGHT, // >> 2 operands
    MATTE_OPERATOR_POW, // ** 2 operands
    MATTE_OPERATOR_EQ, // == 2 operands
    MATTE_OPERATOR_BITWISE_NOT, // ~ 1 operand
    MATTE_OPERATOR_POINT, // -> 2 operands
    MATTE_OPERATOR_POUND, // # 1 operand
    MATTE_OPERATOR_TERNARY, // ? 2 operands
    MATTE_OPERATOR_GREATER, // > 2 operands
    MATTE_OPERATOR_LESS, // < 2 operands
    MATTE_OPERATOR_GREATEREQ, // >= 2 operands
    MATTE_OPERATOR_LESSEQ, // <= 2 operands
    MATTE_OPERATOR_TRANSFORM, // <> 2 operands
    MATTE_OPERATOR_NOTEQ, // != 2 operands
    MATTE_OPERATOR_MODULO, // % 2 operands
    MATTE_OPERATOR_CARET, // ^ 2 operands
    MATTE_OPERATOR_NEGATE, // - 1 operand
    MATTE_OPERATOR_TYPESPEC, // => 2 operands

    // special operators. They arent part of the OPR opcode
    MATTE_OPERATOR_ASSIGNMENT_NONE = 100,
    MATTE_OPERATOR_ASSIGNMENT_ADD,
    MATTE_OPERATOR_ASSIGNMENT_SUB,
    MATTE_OPERATOR_ASSIGNMENT_MULT,
    MATTE_OPERATOR_ASSIGNMENT_DIV,
    MATTE_OPERATOR_ASSIGNMENT_MOD,
    MATTE_OPERATOR_ASSIGNMENT_POW,
    MATTE_OPERATOR_ASSIGNMENT_AND,
    MATTE_OPERATOR_ASSIGNMENT_OR,
    MATTE_OPERATOR_ASSIGNMENT_XOR,
    MATTE_OPERATOR_ASSIGNMENT_BLEFT,
    MATTE_OPERATOR_ASSIGNMENT_BRIGHT,
    
    
    // special operator state flag for OSN instructions.
    // TODO: better method maybe? 
    MATTE_OPERATOR_STATE_BRACKET = 200,


    MATTE_OPERATOR_ERROR = -1,
    
} matteOperator_t;


typedef enum {
    MATTE_EXT_CALL_NOOP,
    MATTE_EXT_CALL_IMPORT,
    MATTE_EXT_CALL_PRINT,
    MATTE_EXT_CALL_SEND,
    MATTE_EXT_CALL_ERROR,
    MATTE_EXT_CALL_BREAKPOINT,
    
    
    
    MATTE_EXT_CALL__NUMBER__PI,    
    MATTE_EXT_CALL__NUMBER__PARSE,
    MATTE_EXT_CALL__NUMBER__RANDOM,


    MATTE_EXT_CALL__STRING__COMBINE,


    MATTE_EXT_CALL__OBJECT__NEWTYPE,
    MATTE_EXT_CALL__OBJECT__INSTANTIATE,
    MATTE_EXT_CALL__OBJECT__FREEZEGC,
    MATTE_EXT_CALL__OBJECT__THAWGC,
    MATTE_EXT_CALL__OBJECT__GARBAGECOLLECT,


    MATTE_EXT_CALL__QUERY__ATAN2,

    MATTE_EXT_CALL__QUERY__PUSH,
    MATTE_EXT_CALL__QUERY__INSERT,
    MATTE_EXT_CALL__QUERY__REMOVE,
    MATTE_EXT_CALL__QUERY__SETATTRIBUTES,
    MATTE_EXT_CALL__QUERY__SORT,
    MATTE_EXT_CALL__QUERY__SUBSET,
    MATTE_EXT_CALL__QUERY__FILTER,
    MATTE_EXT_CALL__QUERY__FINDINDEX,
    MATTE_EXT_CALL__QUERY__ISA,
    MATTE_EXT_CALL__QUERY__MAP,
    MATTE_EXT_CALL__QUERY__REDUCE,
    MATTE_EXT_CALL__QUERY__ANY,
    MATTE_EXT_CALL__QUERY__ALL,
    MATTE_EXT_CALL__QUERY__FOREACH,
    MATTE_EXT_CALL__QUERY__SETSIZE,


    
    MATTE_EXT_CALL__QUERY__CHARAT,
    MATTE_EXT_CALL__QUERY__CHARCODEAT,
    MATTE_EXT_CALL__QUERY__SETCHARAT,
    MATTE_EXT_CALL__QUERY__SETCHARCODEAT,
    MATTE_EXT_CALL__QUERY__SCAN,
    MATTE_EXT_CALL__QUERY__SEARCH,
    MATTE_EXT_CALL__QUERY__SEARCH_ALL,
    MATTE_EXT_CALL__QUERY__FORMAT,
    MATTE_EXT_CALL__QUERY__SPLIT,
    MATTE_EXT_CALL__QUERY__SUBSTR,
    MATTE_EXT_CALL__QUERY__CONTAINS,
    MATTE_EXT_CALL__QUERY__COUNT,
    MATTE_EXT_CALL__QUERY__REPLACE,
    MATTE_EXT_CALL__QUERY__REMOVECHAR,
    
    MATTE_EXT_CALL__QUERY__SET_IS_INTERFACE,




    MATTE_EXT_CALL_GETEXTERNALFUNCTION // always the LAST ext call ID

} matteExtCall_t;



typedef enum {
    MATTE_QUERY__TYPE,

    MATTE_QUERY__COS,
    MATTE_QUERY__SIN,
    MATTE_QUERY__TAN,
    MATTE_QUERY__ACOS,
    MATTE_QUERY__ASIN,
    MATTE_QUERY__ATAN,
    MATTE_QUERY__ATAN2,
    MATTE_QUERY__SQRT,
    MATTE_QUERY__ABS,
    MATTE_QUERY__ISNAN,
    MATTE_QUERY__FLOOR,
    MATTE_QUERY__CEIL,
    MATTE_QUERY__ROUND,
    MATTE_QUERY__RADIANS,
    MATTE_QUERY__DEGREES,
    
    MATTE_QUERY__REMOVECHAR,
    MATTE_QUERY__SUBSTR,
    MATTE_QUERY__SPLIT,
    MATTE_QUERY__SCAN,
    MATTE_QUERY__LENGTH,
    MATTE_QUERY__SEARCH,
    MATTE_QUERY__FORMAT,
    MATTE_QUERY__SEARCH_ALL,
    MATTE_QUERY__CONTAINS,
    MATTE_QUERY__REPLACE,
    MATTE_QUERY__COUNT,
    MATTE_QUERY__CHARCODEAT,
    MATTE_QUERY__CHARAT,
    MATTE_QUERY__SETCHARCODEAT,
    MATTE_QUERY__SETCHARAT,
    
    MATTE_QUERY__KEYCOUNT,
    MATTE_QUERY__SIZE,
    MATTE_QUERY__SETSIZE,
    MATTE_QUERY__KEYS,
    MATTE_QUERY__VALUES,
    MATTE_QUERY__PUSH,
    MATTE_QUERY__POP,
    MATTE_QUERY__INSERT,
    MATTE_QUERY__REMOVE,
    MATTE_QUERY__SETATTRIBUTES,
    MATTE_QUERY__ATTRIBUTES,
    MATTE_QUERY__SORT,
    MATTE_QUERY__SUBSET,
    MATTE_QUERY__FILTER,
    MATTE_QUERY__FINDINDEX,
    MATTE_QUERY__ISA,
    MATTE_QUERY__MAP,
    MATTE_QUERY__REDUCE,
    MATTE_QUERY__ANY,
    MATTE_QUERY__ALL,
    MATTE_QUERY__FOREACH,
    MATTE_QUERY__SET_IS_INTERFACE
} matteQuery_t;

#endif
