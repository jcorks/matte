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
    // push new object with numbered, index keys
    MATTE_OPCODE_NAR,
    // push new object with defined keys and values
    MATTE_OPCODE_NSO,
    
    
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
    MATTE_OPERATOR_TOKEN, // $ 1 operand
    MATTE_OPERATOR_GREATER, // > 2 operands
    MATTE_OPERATOR_LESS, // < 2 operands
    MATTE_OPERATOR_GREATEREQ, // >= 2 operands
    MATTE_OPERATOR_LESSEQ, // <= 2 operands
    MATTE_OPERATOR_TRANSFORM, // <> 2 operands
    MATTE_OPERATOR_NOTEQ, // != 2 operands
    MATTE_OPERATOR_MODULO, // % 2 operands
    MATTE_OPERATOR_CARET, // ^ 2 operands

} matteOperator_t;


typedef enum {
    MATTE_EXT_CALL_NOOP,
    MATTE_EXT_CALL_GATE,
    MATTE_EXT_CALL_LOOP,
    MATTE_EXT_CALL_FOR,
    MATTE_EXT_CALL_FOREACH,
    MATTE_EXT_CALL_MATCH,
    MATTE_EXT_CALL_IMPORT,
    MATTE_EXT_CALL_REMOVE_KEY,
    MATTE_EXT_CALL_TOSTRING, // String() 1 operand
    MATTE_EXT_CALL_TONUMBER, // Number() 1 operand
    MATTE_EXT_CALL_TOBOOLEAN, // Boolean() 1 operand
    MATTE_EXT_CALL_INTROSPECT, // introspect() 1 arg
    MATTE_EXT_CALL_PRINT,
    MATTE_EXT_CALL_ERROR,
    MATTE_EXT_CALL_GETEXTERNALFUNCTION // always the LAST ext call ID

} matteExtCall_t;

#endif
