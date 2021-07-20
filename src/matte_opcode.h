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
    // push new string
    MATTE_OPCODE_NST,
    MATTE_OPCODE_STC,
    // push new empty object
    MATTE_OPCODE_NOB,
    // push new function from a compiled stub
    MATTE_OPCODE_NFN,
    
    
    // push a new callstack from the given function object.
    MATTE_OPCODE_CAL,
    // assign value to a referrable 
    MATTE_OPCODE_ARF,
    // assign object member
    MATTE_OPCODE_OSN,

    // general purpose operator code.
    MATTE_OPCODE_OPR,
    
    // call c / built-in function
    MATTE_OPCODE_EXT,

    // pop values from the stack
    MATTE_OPCODE_POP,
    
    // return;
    MATTE_OPCODE_RET,
    
    // skips PC forward if conditional is false. used for when conditional return
    MATTE_OPCODE_SKP 
        
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
    MATTE_OPERATOR_SHIFT_DOWN, // << 2 operands
    MATTE_OPERATOR_SHIFT_UP, // >> 2 operands
    MATTE_OPERATOR_POW, // ** 2 operands
    MATTE_OPERATOR_EQ, // == 2 operands
    MATTE_OPERATOR_BITWISE_NOT, // ~ 1 operand
    MATTE_OPERATOR_TOSTRING, // .toString() 1 operand
    MATTE_OPERATOR_TONUMBER, // .toNumber() 1 operand
    MATTE_OPERATOR_TOBOOLEAN, // .toBoolean() 1 operand
    MATTE_OPERATOR_CDEREF, // -> 2 operands
    MATTE_OPERATOR_POUND, // # 1 operand
    MATTE_OPERATOR_TERNARY, // ? 2 operands
    MATTE_OPERATOR_CURRENCY, // $ 1 operand
    MATTE_OPERATOR_GREATER, // > 2 operands
    MATTE_OPERATOR_LESS, // < 2 operands
    MATTE_OPERATOR_GREATEREQ, // >= 2 operands
    MATTE_OPERATOR_LESSEQ, // <= 2 operands
    MATTE_OPERATOR_TYPENAME, // .typename() 1 operand 
    MATTE_OPERATOR_SPECIFY, // :: 2 operands
    MATTE_OPERATOR_TRANSFORM, // <> 2 operands
    MATTE_OPERATOR_NOTEQUAL // != 2 operands

} matteOperator_t;


typedef enum {
    MATTE_EXT_CALL_NOOP,
    MATTE_EXT_CALL_GATE,
    MATTE_EXT_CALL_WHILE,
    MATTE_EXT_CALL_FOR3,
    MATTE_EXT_CALL_FOR4,
    MATTE_EXT_CALL_FOREACH,
    MATTE_EXT_CALL_MATCH,
    MATTE_EXT_CALL_GETEXTERNALFUNCTION
} matteExtCall_t;

#endif
