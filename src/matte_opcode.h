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
    // push new empty object
    MATTE_OPCODE_NOB,
    // push new function from a compiled stub
    MATTE_OPCODE_NFN,
    
    
    // push a new callstack from the given function object.
    MATTE_OPCODE_CAL,
    // assign a value 
    MATTE_OPCODE_ASN,
    // general purpose operator code.
    MATTE_OPCODE_OPR
    
    
        
} matteOpcode_t;

#endif
