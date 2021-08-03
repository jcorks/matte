#include "matte_compiler.h"
#include "matte_array.h"
#include "matte_string.h"
#include "matte_table.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>




typedef enum {
    MATTE_TOKEN_EMPTY,

    // token is a number literal
    MATTE_TOKEN_LITERAL_NUMBER,
    MATTE_TOKEN_LITERAL_STRING,
    MATTE_TOKEN_LITERAL_BOOLEAN,
    MATTE_TOKEN_LITERAL_EMPTY,
    MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, // (
    MATTE_TOKEN_EXPRESSION_GROUP_END, // )
    MATTE_TOKEN_ASSIGNMENT,
    MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START,
    MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END,
    MATTE_TOKEN_OBJECT_ACCESSOR_DOT,
    MATTE_TOKEN_GENERAL_OPERATOR1,
    MATTE_TOKEN_GENERAL_OPERATOR2,
    MATTE_TOKEN_VARIABLE_NAME,
    MATTE_TOKEN_OBJECT_STATIC_BEGIN, //{
    MATTE_TOKEN_OBJECT_STATIC_END, //{
    MATTE_TOKEN_OBJECT_DEF_PROP, //:
    MATTE_TOKEN_OBJECT_STATIC_SEPARATOR, //,
    MATTE_TOKEN_OBJECT_ARRAY_START,
    MATTE_TOKEN_OBJECT_ARRAY_END,
    MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR,

    MATTE_TOKEN_DECLARE,
    MATTE_TOKEN_DECLARE_CONST,

    MATTE_TOKEN_FUNCTION_BEGIN, //  
    MATTE_TOKEN_FUNCTION_END,   
    MATTE_TOKEN_FUNCTION_ARG_BEGIN,   
    MATTE_TOKEN_FUNCTION_ARG_SEPARATOR,   
    MATTE_TOKEN_FUNCTION_ARG_END,   
    MATTE_TOKEN_FUNCTION_CONSTRUCTOR, // <<

    MATTE_TOKEN_WHEN,
    MATTE_TOKEN_WHEN_RETURN,
    MATTE_TOKEN_RETURN,

    MATTE_TOKEN_STATEMENT_END, // newline OR ;
    
    MATTE_TOKEN_MARKER_EXPRESSION_END,

} matteTokenType_t;

#define MATTE_TOKEN_END -1



typedef struct matteToken_t matteToken_t ;

struct matteToken_t {
    // type of the token
    matteTokenType_t ttype;

    // optional value for chain construct. Assists in telling 
    // what this token is intended for.
    int chainConstruct;
    
    // source line number
    uint32_t line;
    // char within the line where it starts.

    uint32_t character;
    // text for the token
    matteString_t * text;

    // The next token if this token is part of a chain. Else, is NULL.
    matteToken_t * next;
};



// Tokenizer attempts to 
typedef struct matteTokenizer_t matteTokenizer_t;


// Creates a new tokenizer.
matteTokenizer_t * matte_tokenizer_create(const uint8_t * data, uint32_t len);

// Destroys the tokenizer and all associated matteToken_t * instances.
void matte_tokenizer_destroy(matteTokenizer_t *);

// returns the current line of the tokenizer.
// The initial line is 1.
uint32_t matte_tokenizer_current_line(const matteTokenizer_t *);


// returns the current character of the tokenizer on the current line.
// The initial line is 1.
uint32_t matte_tokenizer_current_character(const matteTokenizer_t *);


// skips space and newlines
int matte_tokenizer_peek_next(matteTokenizer_t *);

// Given a matteTokenType_t, the tokenizer will attempt to parse the next 
// collection of characters as a token of the given type. If the current text 
// is a valid token of that type, a new matteToken_t * is created. If not, 
// NULL is returned. The new token is owned by the matteTokenizer_t * instance and 
// are no longer valid once the matteTokenizer_t * instance is destroyed. 
matteToken_t * matte_tokenizer_next(matteTokenizer_t *, matteTokenType_t);

// Returns whether the tokenizer has reached the end of the inout text.
int matte_tokenizer_is_end(const matteTokenizer_t *);

// prints a token to stdout
void matte_token_print(matteToken_t * t);







typedef struct matteSyntaxGraph_t matteSyntaxGraph_t;
typedef struct matteSyntaxGraphNode_t matteSyntaxGraphNode_t;




enum {
    MATTE_SYNTAX_CONSTRUCT_EXPRESSION = 1, 
    MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION,
    MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,  
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, 
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, 
    MATTE_SYNTAX_CONSTRUCT_VALUE, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS,
    MATTE_SYNTAX_CONSTRUCT_POSTFIX
};



// Creates a new syntax tree,
//
// A syntax graph allows reduction of raw, UT8 text 
// into parsible tokens. 
matteSyntaxGraph_t * matte_syntax_graph_create(
    matteTokenizer_t *,
    uint32_t fileID,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch)
);


// Before a syntax graph can be used, its syntax constructs
// must be registered. Every tree consists of constructs: 
// constructs are symbolic sub-graphs that consist of a series 
// of paths. 
//
// For other functions, referring to constructs is done with 
// the IDs registered here. UTF8 names are associated with them 
// for reporting purposes.
// Each construct ID must be non-zero.
// Terminate with 0.
void matte_syntax_graph_register_constructs(
    matteSyntaxGraph_t *,
    int constructID, const matteString_t * name,
    ...
);


// For defining nodes of the syntax graph, the 
// nodes that accept 
void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t *,
    int token, const matteString_t * name,
    ...
);


// Attempts to add additional tokens to the 
// token chain by scanning for the given constructs.
// returns success (1 if sucess, 0 if failure).
// On failure, the onError function is called with 
// reportable details.
int matte_syntax_graph_continue(
    matteSyntaxGraph_t * graph,
    int constructID
);



void matte_syntax_graph_add_construct_path(
    matteSyntaxGraph_t *,
    const matteString_t * name, 
    int construct,
    ...
);

// returns a new node that represents a single token
matteSyntaxGraphNode_t * matte_syntax_graph_node_token(
    int
);

// returns a new node that prepresents a possible set of 
// token types accepted
matteSyntaxGraphNode_t * matte_syntax_graph_node_token_group(
    int,
    ...
);

// returns a new node that represents the end of a syntax 
// tree path.
matteSyntaxGraphNode_t * matte_syntax_graph_node_end();


// returns a new node that represents a return to 
// a previous node. Level is how far up the tree this 
// previous node is. 0 is the self, 1 is the parent, etc
matteSyntaxGraphNode_t * matte_syntax_graph_node_to_parent(int level);


// returns a new node that represents a construct sub-tree.
matteSyntaxGraphNode_t * matte_syntax_graph_node_construct(int );

// returns a new node that represents a token marker.
// Token markers always succeed in the tree and produce a 
// token.
matteSyntaxGraphNode_t * matte_syntax_graph_node_marker(int );


// returns a new node that represents multiple valid choices 
// along the path. One NULL: ends a path, 2 NULLs: 
matteSyntaxGraphNode_t * matte_syntax_graph_node_split(
    matteSyntaxGraphNode_t *,
    ...
);

// Returns the first parsed token from the user source
matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraph_t * g);

// compiles all nodes into bytecode function blocks.
typedef struct matteFunctionBlock_t matteFunctionBlock_t;
struct matteFunctionBlock_t {
    uint32_t stubID;
    // array of matteString_t *
    matteArray_t * locals;
    // array of matteString_t *
    matteArray_t * args;
    // array of matteBytecodeStubInstruction_t
    matteArray_t * instructions;
    // pointer to parent calling function
    matteFunctionBlock_t * parent;

    // array of matteBytecodeStubCapture_t 
    matteArray_t * captures;
    // array of matteString_t *, names of the captures.
    matteArray_t * captureNames;
};

// converts all graph nodes into an array of matteFunctionBlock_t
matteArray_t * matte_syntax_graph_compile(matteSyntaxGraph_t * g);
static void matte_syntax_graph_print(matteSyntaxGraph_t * g);




uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch),
    uint32_t fileID
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraph_t * st = matte_syntax_graph_create(w, fileID, onError);
    matte_syntax_graph_register_constructs(
        st,
        MATTE_SYNTAX_CONSTRUCT_EXPRESSION, MATTE_STR_CAST("Expression"),
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, MATTE_STR_CAST("Function Call"),
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, MATTE_STR_CAST("Function Scope Statement"),
        MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT, MATTE_STR_CAST("New Object"),
        MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION, MATTE_STR_CAST("New Function"),
        MATTE_SYNTAX_CONSTRUCT_VALUE, MATTE_STR_CAST("Value"),
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS, MATTE_STR_CAST("Function Call Argument List"),
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, MATTE_STR_CAST("New Function Argument List"),
        MATTE_SYNTAX_CONSTRUCT_POSTFIX, MATTE_STR_CAST("Postfix"),
        NULL
    );

    matte_syntax_graph_register_tokens(
        st,
        // token is a number literal
        MATTE_TOKEN_LITERAL_NUMBER, MATTE_STR_CAST("Number Literal"),
        MATTE_TOKEN_LITERAL_STRING, MATTE_STR_CAST("String Literal"),
        MATTE_TOKEN_LITERAL_BOOLEAN, MATTE_STR_CAST("Boolean Literal"),
        MATTE_TOKEN_LITERAL_EMPTY, MATTE_STR_CAST("Empty Literal"),
        MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, MATTE_STR_CAST("Expression '('"),
        MATTE_TOKEN_EXPRESSION_GROUP_END, MATTE_STR_CAST("Expression ')'"),
        MATTE_TOKEN_ASSIGNMENT, MATTE_STR_CAST("Assignment Operator '='"),
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START, MATTE_STR_CAST("Object Accessor '['"),
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END, MATTE_STR_CAST("Object Accessor ']'"),
        MATTE_TOKEN_OBJECT_ACCESSOR_DOT, MATTE_STR_CAST("Object Accessor '.'"),
        MATTE_TOKEN_GENERAL_OPERATOR1, MATTE_STR_CAST("Single-operand Operator"),
        MATTE_TOKEN_GENERAL_OPERATOR2, MATTE_STR_CAST("Operator"),
        MATTE_TOKEN_VARIABLE_NAME, MATTE_STR_CAST("Variable Name"),
        MATTE_TOKEN_OBJECT_STATIC_BEGIN, MATTE_STR_CAST("Object Literal '{'"),
        MATTE_TOKEN_OBJECT_STATIC_END, MATTE_STR_CAST("Object Literal '}'"),
        MATTE_TOKEN_OBJECT_DEF_PROP, MATTE_STR_CAST("Object Literal assignment ':'"),
        MATTE_TOKEN_OBJECT_STATIC_SEPARATOR, MATTE_STR_CAST("Object Literal separator ','"),
        MATTE_TOKEN_OBJECT_ARRAY_START, MATTE_STR_CAST("Array Literal '['"),
        MATTE_TOKEN_OBJECT_ARRAY_END, MATTE_STR_CAST("Array Literal ']'"),
        MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR, MATTE_STR_CAST("Array Element Separator ','"),

        MATTE_TOKEN_DECLARE, MATTE_STR_CAST("Local Variable Declarator '@'"),
        MATTE_TOKEN_DECLARE_CONST, MATTE_STR_CAST("Local Constant Declarator '<@>'"),

        MATTE_TOKEN_FUNCTION_BEGIN, MATTE_STR_CAST("Function Content Block '{'"),
        MATTE_TOKEN_FUNCTION_END,   MATTE_STR_CAST("Function Content Block '}'"),
        MATTE_TOKEN_FUNCTION_ARG_BEGIN, MATTE_STR_CAST("Function Argument List '('"),
        MATTE_TOKEN_FUNCTION_ARG_SEPARATOR, MATTE_STR_CAST("Function Argument Separator ','"),
        MATTE_TOKEN_FUNCTION_ARG_END, MATTE_STR_CAST("Function Argument List ')'"),
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR, MATTE_STR_CAST("Function Constructor '<<'"),

        MATTE_TOKEN_WHEN, MATTE_STR_CAST("'when' Statement"),
        MATTE_TOKEN_WHEN_RETURN, MATTE_STR_CAST("'when' Return Value Operator ':'"),
        MATTE_TOKEN_RETURN, MATTE_STR_CAST("'return' Statement"),

        MATTE_TOKEN_STATEMENT_END, MATTE_STR_CAST("Statement End ';'"),
        NULL
    );


    ///////////////
    ///////////////
    /// VALUE
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Literal Value"), MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_LITERAL_BOOLEAN,
            MATTE_TOKEN_LITERAL_NUMBER,
            MATTE_TOKEN_LITERAL_EMPTY,
            MATTE_TOKEN_LITERAL_STRING,
            0
        ),
        matte_syntax_graph_node_end(),
        NULL
    ); 

    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Variable"), MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_end(),
        NULL
    );




    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Object Constructor"),  MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT),
        matte_syntax_graph_node_end(),
        NULL
    ); 






    ///////////////
    ///////////////
    /// Function Call
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),

        matte_syntax_graph_node_split(
            // no arg path
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
            NULL,

            // one or more args
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                // 1 arg
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(3), // back to arg expression
                NULL,
                NULL
            ),
            NULL,
            NULL
        ),
        NULL
    );
        


    ///////////////
    ///////////////
    /// Expression PostFix
    ///////////////
    ///////////////
    


    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("2-Operand"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );

    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Dot Accessor"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_DOT),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_end(),    
        NULL
    );


    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Assignment"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );


    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Function Call"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );




    ///////////////
    ///////////////
    /// Expression
    ///////////////
    ///////////////




    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Prefix Operator"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR1),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_to_parent(2),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL 
    );
    
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Expression Enclosure"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXPRESSION_GROUP_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END);
        matte_syntax_graph_node_token(MATTE_TOKEN_EXPRESSION_GROUP_END),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_to_parent(2),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL 
    );
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Simple Value"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_to_parent(2),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL 
    );


    ///////////////
    ///////////////
    /// Function Creation Args
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),                        
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),                        
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(4),
                NULL,
                NULL                
            ),
            NULL,
            NULL
        ),
        NULL
    );


    ///////////////
    ///////////////
    /// New Object
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Object Literal Constructor"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_END),
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_construct( MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_END),
                    matte_syntax_graph_node_end(),
                    NULL,
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_SEPARATOR),
                    matte_syntax_graph_node_to_parent(6),
                    NULL,
                    NULL
                ),
                NULL,
                


                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_DEF_PROP),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_END),
                    matte_syntax_graph_node_end(),
                    NULL,
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_SEPARATOR),
                    matte_syntax_graph_node_to_parent(7),
                    NULL,
                    NULL
                ),
                NULL,
                NULL
            ),
            NULL,
            NULL
        ),
        NULL 
    );


    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Function"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
        matte_syntax_graph_node_end(),
        NULL
    );


    // array-like
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Array-like Literal"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_START),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR),
                matte_syntax_graph_node_to_parent(2),
                NULL,
                NULL
                
            ),
            NULL,
            NULL
        ),
        NULL            
    );


    ///////////////
    ///////////////
    /// New Object: function constructor
    ///////////////
    ///////////////

    // function
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_CONSTRUCTOR),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS),
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_BEGIN),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
                    matte_syntax_graph_node_end(),
                    NULL,

                    matte_syntax_graph_node_to_parent(2),
                    NULL,
                    NULL
                ),
                NULL,
                NULL
            ),
            NULL,
            
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_BEGIN),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
                    matte_syntax_graph_node_end(),
                    NULL,

                    matte_syntax_graph_node_to_parent(2),
                    NULL,
                    NULL
                ),
                NULL,
                NULL
            ),  
            NULL,
            NULL
        ),
        NULL            
    );


    ///////////////
    ///////////////
    /// Function Scope Statement
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Return Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("When Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_WHEN),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_WHEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );




    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Declaration + Assignment"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_DECLARE,
            MATTE_TOKEN_DECLARE_CONST,
            0
        ),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
                matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
                matte_syntax_graph_node_end(),
                NULL,


                matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
                matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
                matte_syntax_graph_node_end(),
                NULL,
                NULL 
            ),

            matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
            matte_syntax_graph_node_end(),
            NULL,
            NULL 
        ),
        NULL
    );

    
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Expression"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    
    matte_syntax_graph_add_construct_path(st, MATTE_STR_CAST("Empty Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


   
    // use the text walker to generate an array of tokens for 
    // the entire source. Basic case here allows you to 
    // filter out any poorly formed contextual tokens
    int success;
    while((success = matte_syntax_graph_continue(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT))) {
        if (matte_tokenizer_is_end(w)) break;
    }
    if (!success) {
        *size = 0;
        return NULL;
    }
    matte_syntax_graph_print(st);

    // then walk through the tokens and group them together.
    // a majority of compilation errors will likely happen here, 
    // as only a strict series of tokens are allowed in certain 
    // groups. Might want to form function groups with parenting to track referrables.
    // optimization may happen here if desired.

    // finally emit code for groups.
    matteArray_t * arr = matte_syntax_graph_compile(st);

    //void * bytecode = matte_function_block_array_to_bytecode(arr, size);

    // cleanup :(
    //return bytecode;
}



/////////////////////////////////////////////////////////////

// walker;

static int utf8_next_char(uint8_t ** source) {
    int val = (*source)[0];
    if (val < 128) {
        if (val)(*source)++;
    } else if (val < 224) {
        val += (*source)[1] * 0xff;
        if (val)(*source)+=2;
    } else if (val < 240) {
        val += (*source)[1] * 0xff;
        val += (*source)[2] * 0xffff;
        if (val)(*source)+=3;
    } else {
        val += (*source)[1] * 0xff;
        val += (*source)[2] * 0xffff;
        val += (*source)[3] * 0xffffff;
        if (val)(*source)+=4;
    }
    return val;
}

#define MAX_ASCII_BUFLEN 256

struct matteTokenizer_t {
    uint8_t * backup;
    uint8_t * iter;
    uint8_t * source;
    uint32_t line;
    uint32_t character;
    matteArray_t * tokens;

    char asciiBuffer[MAX_ASCII_BUFLEN+1];
};

static matteToken_t * new_token(
    matteString_t * str,
    uint32_t line, 
    uint32_t character, 
    matteTokenType_t type
) {
    matteToken_t * t = calloc(1, sizeof(matteToken_t));
    t->line = line;
    t->character = character;
    t->ttype = type;
    t->text = str;
    return t;
}

static void destroy_token(
    matteToken_t * t
) {
    if (t->ttype == MATTE_TOKEN_EXPRESSION_GROUP_BEGIN) {
        matte_array_destroy(t->text);
    } else {
        matte_string_destroy(t->text);
    }
    free(t);
}



matteTokenizer_t * matte_tokenizer_create(const uint8_t * data, uint32_t byteCount) {
    matteTokenizer_t * t = calloc(1, sizeof(matteTokenizer_t));
    t->line = 1;
    t->character = 1;
    t->source = malloc(byteCount+sizeof(int32_t));
    uint32_t end = 0;
    memcpy(t->source, data, byteCount);
    memcpy(t->source+byteCount, &end, sizeof(int32_t));
    t->iter = t->source;
    t->backup = t->iter;
    return t;
}

void matte_tokenizer_destroy(matteTokenizer_t * t) {
    uint32_t i;
    uint32_t len = matte_array_get_size(t->tokens);
    for(i = 0; i < len; ++i) {
        matteToken_t * token = matte_array_at(t->tokens, matteToken_t *, i);
        matte_string_destroy(token->text);
        free(token);
    }    
    matte_array_destroy(t->tokens);
    free(t->source);
    free(t);
}

uint32_t matte_tokenizer_current_line(const matteTokenizer_t * t) {
    return t->line;
}

uint32_t matte_tokenizer_current_character(const matteTokenizer_t * t) {
    return t->character;
}


static void tokenizer_strip(matteTokenizer_t * t) {
    int c;
    for(;;) {
        c = utf8_next_char(&t->iter);
        switch(c) {
          case '/': {
            int skipped = 1;
            int cprev = 0;                
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '/': // c-style line comment
                while(c != '\n' && c != 0) {
                    c = utf8_next_char(&t->iter);
                    skipped++;
                }
                // the newline is consumed
                if (c == '\n') {
                    t->line++;
                    t->character = 1;
                }
                t->backup = t->iter;
                break;
              case '*':// c-style block comment
                while(!(c == '/' && cprev == '*') && c != 0) {
                    cprev = c;
                    c = utf8_next_char(&t->iter);
                }
                t->backup = t->iter;
                t->character+=skipped;
                break;
              default: // real char
                t->iter = t->backup;
                break;
            }
            break;
          }
          case '\t':
          case '\r':
          case ' ':
          case '\v':
          case '\f':
            t->backup = t->iter;
            t->character++;
            break;

          case '\n':
            t->backup = t->iter;
            t->line++;
            t->character = 1;
            break;

          // includes 0
          default:;
            t->iter = t->backup;
            return;
        }
    }
}


// skips space and newlines
int matte_tokenizer_peek_next(matteTokenizer_t * t) {
    tokenizer_strip(t);
    uint8_t * iterC = t->iter;  
    return utf8_next_char(&iterC);
}



static matteToken_t * matte_tokenizer_consume_char(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
    matteTokenType_t ty,
    char cha
) {
    int c = utf8_next_char(&t->iter);
    if (cha == c) {
        t->character++;
        t->backup = t->iter;
        return new_token(
            matte_string_create_from_c_str("%c", cha),
            line,
            ch,
            ty
        );            
    } else {
        t->iter = t->backup;
        return NULL;
    }

}

static matteToken_t * matte_tokenizer_consume_word(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
    matteTokenType_t ty,
    const char * word
) {
    uint32_t len = strlen(word);
    uint32_t i;
    int pass = 1;
    for(i = 0; i < len; ++i) {
        if (utf8_next_char(&t->iter) != word[i]) {
            pass = 0;
            break;
        }
    }
    if (pass) {
        t->character += len;
        t->backup = t->iter;
        return new_token(
            matte_string_create_from_c_str("%s", word),
            line,
            ch,
            ty
        );

    } else {
        t->iter = t->backup;
        return NULL;
    }     
}



// Given a matteTokenType_t, the tokenizer will attempt to parse the next 
// collection of characters as a token of the given type. If the current text 
// is a valid token of that type, a new matteToken_t * is created. If not, 
// NULL is returned. The new token is owned by the matteTokenizer_t * instance and 
// are no longer valid once the matteTokenizer_t * instance is destroyed. 
matteToken_t * matte_tokenizer_next(matteTokenizer_t * t, matteTokenType_t ty) {
    tokenizer_strip(t);
    uint32_t currentLine = t->line;
    uint32_t currentCh = t->character;
    // is a marker. Markers dont consume any text.
    if (t->alwaysPass) {
        return new_token(
            matte_string_create_from_c_str(""),
            currentLine,
            currentCh,
            ty
        );      
    }
    switch(ty) {
      case MATTE_TOKEN_LITERAL_NUMBER: {
        // convert into ascii
        int ccount = 0;
        int isDone = 0;
        matteString_t * out = matte_string_create();
        uint8_t * prev;
        while(!isDone) {
            prev = t->iter;
            int c = utf8_next_char(&t->iter);
            switch(c) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4': 
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
              case 'e':
              case 'E':
              case '.':
                matte_string_append_char(out, c);
                break;
              default:
                t->iter = prev;
                isDone = 1;
                break;
            }
        }
        double f;
        if (sscanf(matte_string_get_c_str(out), "%f", &f) == 1) {
            t->character+=matte_string_get_length(out);
            t->backup = t->iter;

            return new_token(
                out, //xfer ownership
                currentLine,
                currentCh,
                ty
            );
        } else {
            matte_string_destroy(out);
            t->iter = t->backup;
            return NULL;
        }
        break;
      }            
    




      case MATTE_TOKEN_LITERAL_STRING: {
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case '\'':
          case '"':
            t->character++;
            break;
          default:
            t->iter = t->backup;
            return NULL;            
        }
    
        
        matteString_t * text = matte_string_create();
        for(;;) {
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '\'':
              case '"':
                // end, cleanup;
                t->backup = t->iter;    
                t->character+=2+matte_string_get_length(text);
                return new_token(
                    text,
                    currentLine,
                    currentCh,
                    ty
                );
                break;
              case 0:
                t->iter = t->backup;
                matte_string_destroy(text);
                return NULL;            

              default:
                matte_string_append_char(text, c);
                break;
            }
        }
        break;
      }



      case MATTE_TOKEN_LITERAL_BOOLEAN: {
        // "true" or "false"
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case 't': // true;
            if (utf8_next_char(&t->iter) == 'r' &&
                utf8_next_char(&t->iter) == 'u' &&
                utf8_next_char(&t->iter) == 'e'
            ) {
                t->character += 4;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("%s", "true"),
                    currentLine,
                    currentCh,
                    ty
                );
            } else {
                t->iter = t->backup;
                return NULL;
            }
            break;
          case 'f': // false;
            if (utf8_next_char(&t->iter) == 'a' &&
                utf8_next_char(&t->iter) == 'l' &&
                utf8_next_char(&t->iter) == 's' &&
                utf8_next_char(&t->iter) == 'e'
            ) {
                t->character += 5;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("%s", "false"),
                    currentLine,
                    currentCh,
                    ty
                );
            } else {
                t->iter = t->backup;
                return NULL;
            }

          default:
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_LITERAL_EMPTY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "empty");
        break;
      }

      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '(');
        break;
      } // (
      case MATTE_TOKEN_EXPRESSION_GROUP_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ')');
        break;

      } // )
      case MATTE_TOKEN_ASSIGNMENT: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '=');
        break;
      }
      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '[');

        break;

      }

      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ']');

        break;          
      }
      case MATTE_TOKEN_OBJECT_ACCESSOR_DOT: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '.');

        break;

      }
      case MATTE_TOKEN_GENERAL_OPERATOR1: {
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case '~':
          case '!':
          case '#':
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("%c", c),
                currentLine,
                currentCh,
                ty
            );            
            break;
          default:
            t->iter = t->backup;
            return NULL;

        }
        break;
      }
      case MATTE_TOKEN_GENERAL_OPERATOR2: { // ugh
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case '+':
          case '/':
          case '?':
          case '%':
          case '^':
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("%c", c),
                currentLine,
                currentCh,
                ty
            );            
            break;


          case '|':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '|':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("|%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("|", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '&':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '&':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("&%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("&", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '<':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '<':
              case '=':
              case '>':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("<%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("<", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '>':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '>':
              case '=':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str(">%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str(">", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;

          case '*':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '*':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("*%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("*", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;

          case '=':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '=':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("=%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return NULL;         
            }
            break;

          case ':':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case ':':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str(":%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return NULL;              
            }
            break;

          case '!':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '=':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("!%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return NULL;              
            }
            break;
          default:
            t->iter = t->backup;
            return NULL;


          case '-':
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '>':
                t->character+=2;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("-%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("-", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


        }
        break;
      }
      case MATTE_TOKEN_VARIABLE_NAME: {
        int c = utf8_next_char(&t->iter);

        // not allowed to start with number
        switch(c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            t->iter = t->backup;
            return NULL;
        }

        int valid = 1;
        matteString_t * varname = matte_string_create();
        while(valid) {
            valid = (c > 48 && c < 58)  || // nums
                    (c > 64 && c < 91)  || // uppercase
                    (c > 96 && c < 123) || // lowercase
                    (c == '_') || // underscore
                    (c > 127); // other things / unicode stuff

            if (valid) {
                t->character++;
                t->backup = t->iter;
                matte_string_append_char(varname, c);
            } else {
                t->iter = t->backup;
                break;
            }
            c = utf8_next_char(&t->iter);
        }
        if (matte_string_get_length(varname)) {
            return new_token(
                varname,
                currentLine,
                currentCh,
                ty
            );
        } else {
            matte_string_destroy(varname);
            return NULL;
        }

        break;
      }
      case MATTE_TOKEN_OBJECT_STATIC_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '{');
        break;          
      }
      case MATTE_TOKEN_OBJECT_STATIC_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '}');
        break;
      }

      case MATTE_TOKEN_OBJECT_DEF_PROP: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ':');
        break;          
      }

      case MATTE_TOKEN_OBJECT_STATIC_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ',');
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '[');
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ']');
        break;          
      }
      case MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ',');       
      }

      case MATTE_TOKEN_DECLARE: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '@');
        break;
      }
      case MATTE_TOKEN_DECLARE_CONST: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "<@>");
        break;          
      }

      case MATTE_TOKEN_FUNCTION_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '{');
        break;
      }
      case MATTE_TOKEN_FUNCTION_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '}');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '(');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ',');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ')');
        break;          
      }
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "<-");
        break;  
      }

      case MATTE_TOKEN_WHEN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "when");
        break;            
      }
      case MATTE_TOKEN_WHEN_RETURN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ':');
        break; 
      }
        
      case MATTE_TOKEN_RETURN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "return");  
        break;
      }

      case MATTE_TOKEN_STATEMENT_END: {// newline OR ;
        int c = utf8_next_char(&t->iter);
        switch(c) {
          /*case '\n':
            t->line++;
            t->character = 1;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("\n"),
                currentLine,
                currentCh,
                ty
            );*/
          case ';':
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(";"),
                currentLine,
                currentCh,
                ty
            );
            break;
          default:
            t->iter = t->backup;
            return NULL;
        }
        break;
      }

      default:
        assert("!NOT HANDLED YET");
    }
}

// Returns whether the tokenizer has reached the end of the inout text.
int matte_tokenizer_is_end(const matteTokenizer_t * t){
    return t->iter[0] == 0;
}


















//////////////////////////////////////////
// syntax graph 

enum {
    MATTE_SYNTAX_GRAPH_NODE__TOKEN,
    MATTE_SYNTAX_GRAPH_NODE__END,
    MATTE_SYNTAX_GRAPH_NODE__SPLIT,
    MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT,
    MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT
};

struct matteSyntaxGraphNode_t {
    // graph_node type
    int type;

    // parent node
    matteSyntaxGraphNode_t * parent;

    // next for cases that use it
    matteSyntaxGraphNode_t * next;

    union {
        struct {
            // number of tokens at this node
            int count;
            int * refs; 
            // if true, always apsses 
            int marker;          
        } token;

        struct {
            // number of valid other node
            int count;
            matteSyntaxGraphNode_t ** nodes;
        } split;

        // how many nodes up 
        int upLevel;

        // id of the construct to assume
        int construct;
    };
};

typedef struct matteSyntaxGraphRoot_t {
    // name of the construct
    matteString_t * name;
    // array of matteSyntaxGraphNode_t *
    matteArray_t * paths;
    // array of matteSyntaxGraphNode_t *
    matteArray_t * pathNames;
} matteSyntaxGraphRoot_t;


struct matteSyntaxGraph_t {
    // first parsed token
    matteToken_t * first;
    // currently last parsed token
    matteToken_t * last;
    // source tokenizer instance
    matteTokenizer_t * tokenizer;
    // which nodes have been attempted so far. This prevents cycles.
    matteTable_t * tried;

    // array of matteString_t *
    matteArray_t * tokenNames;
    // array of matteSYntaxGraphRoot_t *
    matteArray_t * constructRoots;

    // starts at 0 (for the root)
    uint32_t functionStubID;

    uint32_t fileID;

    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch);
};


matteSyntaxGraph_t * matte_syntax_graph_create(
    matteTokenizer_t * t,
    uint32_t fileID;
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch)
) {
    matteSyntaxGraph_t * out = calloc(1, sizeof(matteTokenizer_t));
    out->tokenizer = t;
    out->first = out->last = NULL;
    out->onError = onError;
    out->fileID = fileID;
    out->tokenNames = matte_array_create(sizeof(matteString_t *));
    out->constructRoots = matte_array_create(sizeof(matteSyntaxGraphRoot_t *));
    out->tried = matte_table_create_hash_pointer();
    return out;
}




static void matte_syntax_graph_set_construct_count(
    matteSyntaxGraph_t * g,
    uint32_t id
) {
    while(matte_array_get_size(g->constructRoots) <= id) {
        matteSyntaxGraphRoot_t * root = calloc(1, sizeof(matteSyntaxGraphRoot_t));
        root->name = matte_string_create();
        root->paths = matte_array_create(sizeof(matteSyntaxGraphNode_t *));
        root->pathNames = matte_array_create(sizeof(matteString_t *));
        matte_array_push(g->constructRoots, root);
    }
}

static matteSyntaxGraphRoot_t * matte_syntax_graph_get_root(
    matteSyntaxGraph_t * g,
    uint32_t id
) {
    return matte_array_at(g->constructRoots, matteSyntaxGraphRoot_t *, id);
}




static void matte_syntax_graph_set_token_count(
    matteSyntaxGraph_t * g,
    uint32_t id
) {
    while(matte_array_get_size(g->tokenNames) <= id) {
        matteString_t * name = matte_string_create();
        matte_array_push(g->tokenNames, name);
    }
}

static matteString_t * matte_syntax_graph_get_token_name(
    matteSyntaxGraph_t * g,
    uint32_t id
) {
    return matte_array_at(g->tokenNames, matteString_t *, id);
}





void matte_syntax_graph_register_constructs(
    matteSyntaxGraph_t * g,
    int constructID, const matteString_t * name,
    ...
) {
    matte_syntax_graph_set_construct_count(g, constructID);
    matte_string_set(matte_syntax_graph_get_root(g, constructID)->name, name);

    va_list args;
    va_start(args, name);
    for(;;) {
        int id = va_arg(args, int);
        if (id == 0) break;

        const matteString_t * str = va_arg(args, matteString_t *);
        matte_syntax_graph_set_construct_count(g, id);
        matte_string_set(matte_syntax_graph_get_root(g, id)->name, str);
    }
    va_end(args);
}




// For defining nodes of the syntax graph, the 
// nodes that accept 
void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t * g,
    int token, const matteString_t * name,
    ...
){
    matte_syntax_graph_set_token_count(g, token);
    matte_string_set(matte_syntax_graph_get_token_name(g, token), name);

    va_list args;
    va_start(args, name);
    for(;;) {
        int id = va_arg(args, int);
        if (id == 0) break;

        const matteString_t * str = va_arg(args, matteString_t *);
        matte_syntax_graph_set_token_count(g, id);
        matte_string_set(matte_syntax_graph_get_token_name(g, id), str);
    }
    va_end(args);
}

static void matte_syntax_graph_print_error(
    matteSyntaxGraph_t * graph,
    matteSyntaxGraphNode_t * node
) {
    matteString_t * message = matte_string_create_from_c_str("Syntax Error: Expected ");
    switch(node->type) {
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        uint32_t i;
        uint32_t len = node->token.count;
        if (len == 1) {
            matte_string_concat(message, matte_array_at(graph->tokenNames, matteString_t *, node->token.refs[0]));
        } else {
            for(i = 0; i < len; ++i) {
                if (i == len - 1) {
                    matte_string_concat_printf(message, ", ");
                } else if (i == 0) {

                } else {
                    matte_string_concat_printf(message, ", or ");
                }             
                matte_string_concat(message, matte_array_at(graph->tokenNames, matteString_t *, node->token.refs[i]));
            }
        }
        break;
      }
      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(graph, node->construct);
        matte_string_concat(message, root->name);
        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__SPLIT: {
      }


      default:
        matte_string_concat_printf(message, "[a series of tokens i was too lazy to write the logic to print yet]");
      
    }

    matte_string_concat_printf(message, " but received '");
    matte_string_append_char(message, matte_tokenizer_peek_next(graph->tokenizer));
    matte_string_concat_printf(message, "' instead.");


    graph->onError(
        message,
        matte_tokenizer_current_line(graph->tokenizer),
        matte_tokenizer_current_character(graph->tokenizer)
    );
    matte_string_destroy(message);
}

#ifdef MATTE_DEBUG
static void print_graph_node(matteSyntaxGraph_t * g, matteSyntaxGraphNode_t * n) {
    if (!n) {
        printf("<null>\n");
        return;
    }
    
    switch(n->type) {
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        printf("Token node:\n");
        printf("    tokenCount: %d\n", n->token.count);
        uint32_t i;
        uint32_t len = n->token.count;
        for(i = 0; i < len; ++i) {
            printf("    accepted: %s\n", matte_string_get_c_str(matte_syntax_graph_get_token_name(g, n->token.refs[i])));
        }
        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__SPLIT: {
        printf("Split node:\n");
        printf("    pathCount: %d\n", n->split.count);
        printf("    accepted paths:\n[[[[[[---\n");
        uint32_t i;
        uint32_t len = n->split.count;
        for(i = 0; i < len; ++i) {
            print_graph_node(g, n->split.nodes[i]);            
        }
        printf("---]]]]]]\n");

        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT: {
        printf("Parent-redirect node:\n");
        printf("    level?: %d\n", n->upLevel);
        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__END: {
        printf("End node.\n");
        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        printf("Construct node. (%s)\n", matte_string_get_c_str(matte_syntax_graph_get_root(g, n->construct)->name));
        break;
      }

    }
}

#endif


// attempts to consume the current node and returns 
// the next node. If the node is not valid, NULL is returned.
static matteSyntaxGraphNode_t * matte_syntax_graph_walk(
    matteSyntaxGraph_t * graph,
    matteSyntaxGraphNode_t * node,
    int constructID,
    int silent
) {
    if (matte_table_find(graph->tried, node)) return NULL;
    matte_table_insert(graph->tried, node, (void*)0x1);
    #ifdef MATTE_DEBUG
        static int level = 0;
        level++;
    #endif
    
    switch(node->type) {
      // node is simply a token / token set.
      // We try each token in order. If it parses successfully,
      // we continue. If not, we try the next token.
      // If we exhaust all tokens, we have failed.
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        #ifdef MATTE_DEBUG
            int h; for(h = 0; h < level; ++h) printf("@");
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__TOKEN: %s (next c == '%c')\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);
        #endif
        uint32_t i;
        uint32_t len = node->token.count;
        for(i = 0; i < len; ++i) {
            #ifdef MATTE_DEBUG
                printf("     - trying to parse token as %s...", matte_string_get_c_str(matte_array_at(graph->tokenNames, matteString_t *, node->token.refs[i])));
            #endif
            matteToken_t * newT = matte_tokenizer_next(graph->tokenizer, node->token.refs[i]);
            #ifdef MATTE_DEBUG
                printf("%s\n", newT ? "SUCCESS!": "failure");
            #endif

            // success!!!
            if (newT) {
                newT->chainConstruct = constructID;
                if (!graph->first) {
                    graph->first = newT;
                    graph->last = newT;
                } else {
                    graph->last->next = newT;
                    graph->last = newT;
                }
                matte_table_clear(graph->tried);
                #ifdef MATTE_DEBUG
                    level--;
                #endif
                return node->next;
            }
        }
        // failure
        if (!silent)
            matte_syntax_graph_print_error(graph, node);
        #ifdef MATTE_DEBUG
            level--;
        #endif
        return NULL;
      }


      // the node is generically able to be split into 
      // possible paths. Each are attempted in order.
      case MATTE_SYNTAX_GRAPH_NODE__SPLIT: {
        #ifdef MATTE_DEBUG
            int h; for(h = 0; h < level; ++h) printf("@");
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__SPLIT %s (next c == '%c')\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif

        uint32_t i;
        uint32_t len = node->split.count;
        for(i = 0; i < len; ++i) {
            matteSyntaxGraphNode_t * n = matte_syntax_graph_walk(
                graph,
                node->split.nodes[i],
                constructID,
                1
            );
            // success!
            if (n) {
                matte_table_clear(graph->tried);
                #ifdef MATTE_DEBUG
                    level--;
                #endif
                return n;
            }
        }
        // failure
        if (!silent)
            matte_syntax_graph_print_error(graph, node);
        #ifdef MATTE_DEBUG
            level--;
        #endif
        return NULL;
        break;
      }


      // the node gener
      case MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT: {
        #ifdef MATTE_DEBUG
            printf("WALKING: @MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT %s (next c == '%c')\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif

        uint32_t level = node->upLevel;
        matteSyntaxGraphNode_t * n = node;
        while(level--) {
            if (n->parent) n = n->parent;
        }
        if (!n) {
            if (!silent)
                matte_syntax_graph_print_error(graph, node);
            #ifdef MATTE_DEBUG
                level--;
            #endif
            return NULL;
        } else {
            matte_table_clear(graph->tried);
            #ifdef MATTE_DEBUG
                level--;
            #endif
            return n;
        }
      }

      // the end of a path has been reached. return
      case MATTE_SYNTAX_GRAPH_NODE__END: {
        #ifdef MATTE_DEBUG
            int h; for(h = 0; h < level; ++h) printf("@");
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__END %s (next c == '%c')\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);
            matte_table_clear(graph->tried);
        #endif
        #ifdef MATTE_DEBUG
            level--;
        #endif
        return node;
      }


      // the node is a construct path
      // All top paths are tried before continuing
      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        #ifdef MATTE_DEBUG
            int h; for(h = 0; h < level; ++h) printf("@");
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT %s (next c == '%c')\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(graph, node->construct);

        uint32_t i;
        uint32_t len = matte_array_get_size(root->paths);
        for(i = 0; i < len; ++i) {
            matteSyntaxGraphNode_t * tr = matte_array_at(root->paths, matteSyntaxGraphNode_t * , i);
            matteSyntaxGraphNode_t * out = matte_syntax_graph_walk(graph, tr, node->construct, 1);
            if (out) {
                matte_table_clear(graph->tried);
                #ifdef MATTE_DEBUG
                    int h; for(h = 0; h < level; ++h) printf("@");
                    printf("  - PASSED initial construct node for path %s\n", matte_string_get_c_str(matte_array_at(root->pathNames, matteString_t *, i)));
                    fflush(stdout);

                #endif
                while(out = matte_syntax_graph_walk(graph, out, node->construct, 0)) {
                    // The only way to validly finish a path
                    if (out && out->type == MATTE_SYNTAX_GRAPH_NODE__END) {
                        matte_table_clear(graph->tried);
                        #ifdef MATTE_DEBUG
                            level--;
                        #endif
                        return node->next;
                    }
                }
            } else {
                #ifdef MATTE_DEBUG
                    int h; for(h = 0; h < level; ++h) printf("@");
                    printf("  - FAILED initial construct node for path %s\n", matte_string_get_c_str(matte_array_at(root->pathNames, matteString_t *, i)));
                    fflush(stdout);

                #endif
            }
        }
        if (!silent)
            matte_syntax_graph_print_error(graph, node);

        #ifdef MATTE_DEBUG
            level--;
        #endif
        return NULL;

        break;
      }


    }
    assert(!"should not reach here");
}


int matte_syntax_graph_continue(
    matteSyntaxGraph_t * graph,
    int constructID
) {
    if (constructID < 0 || constructID >= matte_array_get_size(graph->constructRoots)) {
        graph->onError(MATTE_STR_CAST("Internal error (no such constrctID)"), 0, 0);
        return 0;
    }

    matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(graph, constructID);
    matteSyntaxGraphNode_t topNode = {0};
    topNode.type = MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT; 
    topNode.construct = constructID;

    matteSyntaxGraphNode_t endNode = {0};
    endNode.type = MATTE_SYNTAX_GRAPH_NODE__END;

    topNode.next = &endNode;



    matteSyntaxGraphNode_t * next = &topNode;
    while(next = matte_syntax_graph_walk(graph, next, constructID, 0)) {
        // The only way to validly finish a path
        if (next && next->type == MATTE_SYNTAX_GRAPH_NODE__END) {
            return 1;
            break;          
        }
    }
    return 0;
}



void matte_syntax_graph_add_construct_path(
    matteSyntaxGraph_t * g,
    const matteString_t * name,
    int constructID,
    ...
){
    matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(g, constructID);
    matteSyntaxGraphNode_t * n;
    matteSyntaxGraphNode_t * prev = NULL;
    va_list args;
    va_start(args, constructID);
    for(;;) {
        n = va_arg(args, matteSyntaxGraphNode_t *);
        if (n) {
            if (prev) {
                n->parent = prev;
                if (prev->type == MATTE_SYNTAX_GRAPH_NODE__TOKEN ||
                    prev->type == MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT)
                    prev->next = n;
            } else {
                matte_array_push(root->paths, n);
                matteString_t * cp = matte_string_clone(name);
                matte_array_push(root->pathNames, cp);
            }
        } else {
            // done
            break;
        }
        prev = n;
    }
    va_end(args);
}

// returns a new node that represents a single token
matteSyntaxGraphNode_t * matte_syntax_graph_node_token(
    int a
){
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN;
    n->token.count = 1;
    n->token.refs = malloc(sizeof(int));
    n->token.refs[0] = a;
    n->token.alwaysPass = 0;
    return n;
}



// returns a new node that represents a single token
matteSyntaxGraphNode_t * matte_syntax_graph_node_marker(
    int a
){
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN;
    n->token.count = 1;
    n->token.refs = malloc(sizeof(int));
    n->token.refs[0] = a;
    n->token.alwaysPass = 1;
    return n;
}



// returns a new node that prepresents a possible set of 
// token types accepted
matteSyntaxGraphNode_t * matte_syntax_graph_node_token_group(
    int a,
    ...
){
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN;


    va_list args;
    va_start(args, a);

    matteArray_t * tokens = matte_array_create(sizeof(int));
    for(;;) {
        if (a) 
            matte_array_push(tokens, a);
        else 
            break;
        a = va_arg(args, int);
    }
    uint32_t i;
    n->token.count = matte_array_get_size(tokens);
    n->token.refs = malloc(sizeof(int)*n->token.count);
    for(i = 0; i < n->token.count; ++i) {
        n->token.refs[i] = matte_array_at(tokens, int, i);
    }
    va_end(args);
    matte_array_destroy(tokens);
    assert(n->token.count);
    return n;
}

// returns a new node that represents the end of a syntax 
// tree path.
matteSyntaxGraphNode_t * matte_syntax_graph_node_end() {
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__END;
    return n;
}


// returns a new node that represents a return to 
// a previous node. Level is how far up the tree this 
// previous node is. 0 is the self, 1 is the parent, etc
matteSyntaxGraphNode_t * matte_syntax_graph_node_to_parent(int level) {
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT;
    n->upLevel = level;
    return n;  
}


// returns a new node that represents a construct sub-tree.
matteSyntaxGraphNode_t * matte_syntax_graph_node_construct(int a){
    matteSyntaxGraphNode_t * n = calloc(1, sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT;
    n->construct = a;
    return n;  
}

// returns a new node that represents multiple valid choices 
// along the path. One NULL: ends a path, 2 NULLs: 
matteSyntaxGraphNode_t * matte_syntax_graph_node_split(
    matteSyntaxGraphNode_t * n,
    ...
) {
    matteSyntaxGraphNode_t * out = calloc(1, sizeof(matteSyntaxGraphNode_t));
    out->type = MATTE_SYNTAX_GRAPH_NODE__SPLIT;

    matteSyntaxGraphNode_t * prev = NULL;
    matteSyntaxGraphNode_t * path = n;
    n->parent = out;
    matteArray_t * paths = matte_array_create(sizeof(matteSyntaxGraphNode_t *));
    va_list args;
    va_start(args, n);
    for(;;) {
        if (n) {
            if (prev) {
                n->parent = prev;
                if (prev->type == MATTE_SYNTAX_GRAPH_NODE__TOKEN ||
                    prev->type == MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT)
                    prev->next = n;
            } else {
                n->parent = out;
            }
        } else {
            n = va_arg(args, matteSyntaxGraphNode_t *);
            prev = NULL;
            // true end
            assert(path);
            matte_array_push(paths, path);            
            path = NULL;
            if (!n) {
                break;
            } 
            path = n;
            n->parent = out;
        }
        prev = n;
        n = va_arg(args, matteSyntaxGraphNode_t *);

    }
    va_end(args);

    uint32_t i;
    uint32_t len = matte_array_get_size(paths);
    out->split.count = len;
    out->split.nodes = malloc(sizeof(matteSyntaxGraphNode_t *)*len);
    for(i = 0; i < len; ++i) {
        out->split.nodes[i] = matte_array_at(paths, matteSyntaxGraphNode_t *, i);
    }
    matte_array_destroy(paths);
    assert(len);
    return out;
}

matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraph_t * g) {
    return g->first;
}



static void matte_token_print__helper(matteSyntaxGraph_t * g, matteToken_t * t) {
    if (t == NULL) {
        printf("<token is NULL>\n");
        fflush(stdout);
        return;
    }
    printf("line:%5d col:%4d    %-40s'%s'\n", 
        t->line,
        t->character,
        matte_string_get_c_str(matte_syntax_graph_get_token_name(g, t->ttype)),
        matte_string_get_c_str(t->text)
    );


}

static void matte_syntax_graph_print(matteSyntaxGraph_t * g) {
    printf("Level0 Token sequence:\n");
    matteToken_t * t = g->first; 
    do {
        matte_token_print__helper(g, t);
        t = t->next;
    } while(t);
    fflush(stdout);

}









///////////// compilation 

static void matte_syntax_graph_print_compile_error(
    matteSyntaxGraph_t * graph,
    matteToken_t * t,
    const char * asciiMessage
) {
    matteString_t * message = matte_string_create_from_c_str("Compile Error: %s", asciiMessage);
    graph->onError(
        message,
        t->line,
        t->character
    );
    matte_string_destroy(message);
}

// fast forwards the token chain past ALL inner functions.
// by the end, should be on }
static void ff_skip_inner_function(matteToken_t ** t) {
    // assumes starting at function constructor <= 
    matteToken_t * iter = (*t)->next;
    for(;;) {
        switch(iter->ttype) {
          case MATTE_TOKEN_FUNCTION_CONSTRUCTOR:
            ff_skip_inner_function(&iter);
            break;
          case MATTE_TOKEN_FUNCTION_END:
            *t = iter;
            return;
          default:;
        }
        iter = iter->next;
        // should never happen.
        if (!iter) {
            return;
        }
    }

}

void destroy_function_block(matteFunctionBlock_t * b) {
    uint32_t i;
    uint32_t len;

    len = matte_array_get_size(b->args);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(b->args, matteString_t *, i));
    }

    len = matte_array_get_size(b->locals);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(b->locals, matteString_t *, i));
    }
    len = matte_array_get_size(b->captureNames);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(b->captureNames, matteString_t *, i));
    }

    matte_array_destroy(b->args);
    matte_array_destroy(b->locals);
    matte_array_destroy(b->instructons);
    matte_array_destroy(b->captures);
    matte_array_destroy(b->captureNames);
    free(b);
}

#include "MATTE_INSTRUCTIONS"


// returns 0xffffffff
static uint32_t get_local_referrable(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * block,
    matteToken_t * iter,
) {
    uint32_t i;
    uint32_t len;

    // special: always refers to the calling context.
    if (matte_string_test_eq(iter->text, MATTE_STR_CAST("context"))) {
        return 0;
    }

    //while(block) {
    len = matte_array_get_size(block->args);
    for(i = 0; i < len; ++i) {
        if (matte_string_test_eq(iter->text, matte_array_at(block->args, matteString_t *, i))) {
            return i+1;
        }
    }

    uint32_t offset = len+1;
    len = matte_array_get_size(block->locals);
    for(i = 0; i < len; ++i) {
        if (matte_string_test_eq(iter->text, matte_array_at(block->locals, matteString_t *, i))) {
            return offset+i;
        }
    }

    //block = block->parent;
    //}

    matteString_t * m = matte_string_create_from_c_str("Could not find local referrable of the given name '%s'", iter->text);
    matte_syntax_graph_print_compile_error(g, iter, matte_string_get_c_str(m));
    matte_string_destroy(m);
    return 0xffffffff;
}



// attempts to return an array of additional instructions to push a variable onto the stack.
// returns 0 on failure
static matteArray_t * push_variable_name(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * block,
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * inst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

    uint32_t referrable = get_local_referrable(
        g,
        block,
        iter
    );
    if (referrable != 0xffffffff) {
        write_instruction__prf(inst, iter->lineNumber, referrable);
        return inst;
    } else {
        // first look through existing captures
        uint32_t i;
        uint32_t len;

        len = matte_array_get_size(g->captureNames);
        for(i = 0; i < len; ++i) {
            if (matte_string_test_eq(iter->text, matte_array_at(g->captureNames, matteString_t *, i))) {
                write_instruction__prf(inst, iter->lineNumber, i+1+matte_array_get_size(g->locals)+matte_array_get_size(g->args));
                *src = iter->next;
                return inst;                    
            }
        }

        // if not found, walk through function scopes 
        //      if found, append to existing captures
        matteFunctionBlock_t * blockSrc = block;
        block = block->parent;
        while(block) {

            len = matte_array_get_size(block->args);
            for(i = 0; i < len; ++i) {
                if (matte_string_test_eq(iter->text, matte_array_at(block->args, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = blockSrc->stubID;
                    capture.referrable = i+1;
                    matte_array_push(blockSrc->captures, capture);
                    write_instruction__prf(
                        inst, iter->lineNumber, 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matteString_t * str = matte_string_clone(iter->text);
                    matte_array_push(blockSrc->captureNames, iter->text);
                    *src = iter->next;
                    return inst;                    
                }
            }

            uint32_t offset = len+1;
            len = matte_array_get_size(block->locals);
            for(i = 0; i < len; ++i) {
                if (matte_string_test_eq(iter->text, matte_array_at(block->locals, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = blockSrc->stubID;
                    capture.referrable = i+offset;
                    matte_array_push(blockSrc->captures, capture);
                    write_instruction__prf(
                        blockSrc, iter->lineNumber, 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matteString_t * str = matte_string_clone(iter->text);
                    matte_array_push(blockSrc->captureNames, iter->text);
                    *src = iter->next;
                    return inst;                    

                }
            }
            block = block->parent;
        }
    }


    matteString_t * m = matte_string_create_from_c_str("Undefined variable '%s'", iter->text);
    matte_syntax_graph_print_compile_error(g, iter, matte_string_get_c_str(m));
    matte_string_destroy(m);

    matte_array_destroy(inst);
    return NULL;
}


matteOperator_t string_to_operator(const matteString_t * s) {
    int opopcode0 = matte_string_get_char(s, 0);
    int opopcode1 = matte_string_get_char(s, 1);
    switch(opopcode0) {
        case '+': return MATTE_OPERATOR_ADD; break;
        case '-': 
        switch(opopcode1) {
            case '>': return MATTE_OPERATOR_POINT; break;
            default:;
        }
        return MATTE_OPERATOR_SUB; 
        break;

        case '/': return MATTE_OPERATOR_DIV; break;
        case '*': 
        switch(opopcode1) {
            case '*': return MATTE_OPERATOR_POW; break;
            default:;
        }
        return MATTE_OPERATOR_MULT;
        break;

        case '!': 
        switch(opopcode1) {
            case '=': return MATTE_OPERATOR_NOTEQ; break;
            default:;
        }
        return MATTE_OPERATOR_NOT;
        break;
        case '|': 
        switch(opopcode1) {
            case '|': return MATTE_OPERATOR_OR; break;
            default:;
        }
        return MATTE_OPERATOR_BITWISE_OR;
        break;
        case '&':
        switch(opopcode1) {
            case '&': return MATTE_OPERATOR_AND; break;
            default:;
        }
        return MATTE_OPERATOR_BITWISE_AND;
        break;

        case '<':
        switch(opopcode1) {
            case '<': return MATTE_OPERATOR_SHIFT_LEFT; break;
            case '=': return MATTE_OPERATOR_LESSEQ; break;
            case '>': return MATTE_OPERATOR_TRANSFORM; break;
            default:;
        }
        return MATTE_OPERATOR_LESS;
        break;

        case '>':
        switch(opopcode1) {
            case '<': return MATTE_OPERATOR_SHIFT_RIGHT; break;
            case '=': return MATTE_OPERATOR_GREATEREQ; break;
            default:;
        }
        return MATTE_OPERATOR_GREATER;
        break;

        case '=':
        switch(opopcode1) {
            case '=': return MATTE_OPERATOR_EQ; break;
            default:;
        }
        break;

        case '~': return MATTE_OPERATOR_BITWISE_NOT; break;
        case 'T':
        switch(opopcode1) {
            case 'S': return MATTE_OPERATOR_TOSTRING; break;
            case 'N': return MATTE_OPERATOR_TONUMBER; break;
            case 'B': return MATTE_OPERATOR_TOBOOLEAN; break;
            case 'Y': return MATTE_OPERATOR_TYPENAME; break;
            default:;
        }
        break;

        case '#': return MATTE_OPERATOR_POUND; break;
        case '?': return MATTE_OPERATOR_TERNARY; break;
        case '$': return MATTE_OPERATOR_TOKEN; break;
        case '^': return MATTE_OPERATOR_CARET; break;
        case '%': return MATTE_OPERATOR_MODULO; break;

        case ':':
        switch(opopcode1) {
            case ':': return MATTE_OPERATOR_SPECIFY; break;
            default:;
        }
        break;
        default:
    }
    return -1;
}


// pushes all instructions within the array and destroys the array
static void write_instructions(
    matteFunctionBlock_t * g, 
    matteArray_t * arr    
) {
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    for(i = 0; i < len; ++i) {
        matte_aray_push(g->instructions, matte_array_at(arr, matteBytecodeStubInstruction_t, i));
    }
    matte_array_destroy(arr);
}


static void merge_instructions(
    matteArray_t * m,
    matteArray_t * arr
) {
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    for(i = 0; i < len; ++i) {
        matte_aray_push(m, matte_array_at(arr, matteBytecodeStubInstruction_t, i));
    }
    matte_array_destroy(arr);
}


typedef struct matteExpressionNode_t matteExpressionNode_t;
struct matteExpressionNode_t {
    // 1-operand operator for this node, if any
    // -1 for none
    // preOp is applied right before the value is used, SO it doesnt 
    // play a direct role in determining precedence (and hence, value resolution)
    int preOp; 
    
    // 2-operand operator for this node and the next
    // -1 for none;
    matteOperator_t postOp;

    // starts at 0 for a same-level expression
    int appearanceID;

    // precedence level
    int precedence;


    // array of instructions that, when run, push the vaue in question
    matteArray_t * value; 

    // next and prev nodes
    // PLEASE NOTE: these are the COMPUTATIONALLY SURROUNDED nodes
    // nodes are not computed from left to right but are resorted and 
    // collapse neighboring nodes in order of precedence
    matteExpressionNode_t * next;
    matteExpressionNode_t * prev;

};

// more or less C!
//
// Though.... i have a vague feeling that evil lurks here...
// an old, inherent evil...
static int op_to_precedence(int op) {
    switch(op) {
      case MATTE_OPERATOR_POUND: // # 1 operand
      case MATTE_OPERATOR_TOKEN: // $ 1 operand
        return 0;
      case MATTE_OPERATOR_POINT: // -> 2 operands
        return 1;

      case MATTE_OPERATOR_ADD:
      case MATTE_OPERATOR_SUB:
        return 2;

      case MATTE_OPERATOR_NOT:
      case MATTE_OPERATOR_BITWISE_NOT:
        return 3;

      case MATTE_OPERATOR_DIV:
      case MATTE_OPERATOR_MULT:
      case MATTE_OPERATOR_MODULO: // % 2 operands
        return 4; 


      case MATTE_OPERATOR_SHIFT_LEFT: // << 2 operands
      case MATTE_OPERATOR_SHIFT_RIGHT: // >> 2 operands
      case MATTE_OPERATOR_POW, // ** 2 operands
        return 5;

      case MATTE_OPERATOR_GREATER: // > 2 operands
      case MATTE_OPERATOR_LESS: // < 2 operands
      case MATTE_OPERATOR_GREATEREQ: // >= 2 operands
      case MATTE_OPERATOR_LESSEQ: // <= 2 operands
        return 6;

      case MATTE_OPERATOR_EQ: // == 2 operands
      case MATTE_OPERATOR_NOTEQ: // != 2 operands
        return 7;

      case MATTE_OPERATOR_BITWISE_AND: // & 2 operands
        return 8;


      case MATTE_OPERATOR_BITWISE_OR: // | 2 operands
        return 10;

      case MATTE_OPERATOR_AND: // && 2 operands
        return 11;

      case MATTE_OPERATOR_OR:
        return 12;


      case MATTE_OPERATOR_TERNARY:
      case MATTE_OPERATOR_SPECIFY: // :: 2 operands
      case MATTE_OPERATOR_TRANSFORM: // <> 2 operands
      case MATTE_OPERATOR_CARET: // ^ 2 operands

        return 13;
    }
    return -1;
}

static matteExpressionNode_t * new_expression_node(
    int preOp,
    int postOp,
    int appearanceID,
    // xfer ownership
    matteArray_t * value
) {
    matteExpressionNode_t * out = calloc(1, sizeof(matteExpressionNode_t));
    out->preOp = preOp;
    out->postOp = postOp;
    out->appearanceID = appearanceID;
    out->precedence = op_to_precedence(out->postOp);
    out->value = value;
    return out;
}



static int expression_node_sort__comparator(
    const void * aSrc,
    const void * bSrc,
) {
    matteExpressionNode_t * a = *(matteExpressionNode**)aSrc;
    matteExpressionNode_t * b = *(matteExpressionNode**)bSrc;

    if (a->precedence < b->precedence) {
        return -1;
    } else if (a->precedence > b->precedence) {
        return 1;
    } else {
        if (a->appearanceID < b->appearanceID) return -1;
        return 1;
    }
}

static void expression_node_sort(matteArray_t * nodes) {
    qsort(
        matte_array_get_data(nodes),
        matte_array_get_size(nodes),
        sizeof(matteExpressionNode_t *),
        expression_node_sort__comparator
    );
}


// like compile expression, except the expression has 
// no operators and is a "simple" value.
// Values in this context include:
// - literals
// - new objects 
//
// Returned is an array of instructions that, if run
// push exactly one value on the stack 
static matteArray_t * compile_value(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * inst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

    switch(iter->ttype) {
      case MATTE_TOKEN_LITERAL_BOOLEAN:
        write_instruction__nbl(inst, inst->line, matte_string_test_eq(iter->text, MATTE_STR_CAST("true")));
        *src = iter->next;
        return inst;

      case MATTE_TOKEN_LITERAL_NUMBER: {
        double val = 0.0;
        sscanf(matte_string_get_c_str(iter->text, &val));
        write_instruction__nnm(inst, inst->lineNumber, val);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_LITERAL_EMPTY: {
        write_instruction__nem(inst, iter->lineNumber, val);
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_LITERAL_STRING: {
        write_instruction__nst_stc(inst, iter->lineNumber, iter->text);
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_VARIABLE_NAME: {
        matte_array_destroy(inst);
        return push_variable_name(
            g, 
            block,
            src
        );
      }


      // array literal
      case MATTE_TOKEN_OBJECT_ARRAY_START: {
        iter = iter->next;
        uint32_t itemCount = 0;
        while(iter->ttype != MATTE_TOKEN_OBJECT_ARRAY_END) {
            matteArray_t * expInst = compile_expression(
                g,
                block,
                functions,
                &iter
            );
            if (!expInst) {
                matte_array_destroy(inst);
                return NULL;                
            }
            iter = iter->next; // skip ,
            merge_instructions(inst, expInst);
            itemCount++;
        }
        write_instruction__nar(inst, iter->lineNumber, itemCount);
        *src = iter->next; // skip ]
        return inst;
        break;
      }



      // object literal
      case MATTE_TOKEN_OBJECT_STATIC_BEGIN: {
        iter = iter->next;
        uint32_t nPairs = 0;
        while(iter->ttype != MATTE_TOKEN_OBJECT_STATIC_END) {
            // key : value,

            // special case: variable name is treated like a string for convenience
            if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME &&
                iter->next->ttype == MATTE_TOKEN_OBJECT_DEF_PROP) {

                write_instruction__nst_stc(inst, iter->lineNumber, iter->text);
                iter = iter->next; // skip name
                


            // normal case, expression key
            } else {
                matteArray_t * expInst = compile_expression(
                    g,
                    block,
                    functions,
                    &iter
                );
                if (!expInst) {
                    matte_array_destroy(inst);
                    return NULL;                
                }
                merge_instructions(inst, expInst);
            }
            iter = iter->next; // skip : 



            matteArray_t * expInst = compile_expression(
                g,
                block,
                functions,
                &iter
            );
            if (!expInst) {
                matte_array_destroy(inst);
                return NULL;                
            }
            merge_instructions(inst, expInst);


            if (inst->ttype == MATTE_TOKEN_OBJECT_STATIC_SEPARATOR)
                inst = inst->next;                
            nPairs++;
        }
        
        write_instruction__nso(inst, iter->lineNumber, nPairs);
        *src = iter->next; // skip } 
        return inst;
      }


      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: { 
        matteArray_t * arr = (matteArray_t *)iter->text; // the sneaky in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }

      case MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION: {
        int val;
        sscanf(matte_string_get_c_str(iter->text), "%d", &val);
        write_instruction__nfn(
            inst, 
            iter->lineNumber,
            g->fileID,
            val
        );
        *src = iter->next;
        return inst;
      }
    }

    matte_syntax_graph_print_compile_error(g, iter, "Unrecognized value token (internal error)");
    matte_array_destroy(inst);
    return NULL;               

}


// Returns an array of instructions that, when computed in order,
// produce exactly ONE value on the stack (the evaluation of the expression).
// returns NULL on failure. Expected to report errors 
// when encountered.
// The MATTE_TOKEN_MARKER_EXPRESSION_END token is consumed.
static matteArray_t * compile_expression(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * outInst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    matteArray_t * nodes = matte_array_create(sizeof(matteExpressionNode_t *));

    // paranetheticals should be evaluated first.
    // also!! functions are always constructed as part of 
    // expressions. In Matte function constructors <-> definitions
    while(iter && iter->ttype != MATTE_TOKEN_MARKER_EXPRESSION_END) {
        switch(iter->ttype) {
          case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {
            matteToken_t * start = iter;
            matteFunctionBlock_t * b = compile_function_block(g, b, functions, &iter);
            if (!b) {
                goto L_FAIL;
            }
            // to reference this new function as needed, the token for the 
            // function constructor will have its text modified to be an integer to the 
            matte_string_clear(start->text);
            matte_string_concat_printf(start->text, "%d", b->stubID); // assumes same fileID

            matteToken_t * end = iter;
            matte_array_push(functions, b);
            // because we'll be revisiting these nodes, the internal tokens 
            // that consist of the function definition should be removed from the list.
            iter = start->next;
            matteToken_t * next;
            while(iter != end) {
                next = iter->next;
                destroy_token(iter);            
                iter = next;
            }
            start->next = end;
            break;
          }
          
          case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN:
            matteToken_t * start = iter; 
            iter = iter->next; // skip (
            matteArray_t * inst = compile_expression(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }
            matteToken_t * end = iter->next; // skip )
            
            // to reference this expressions value, the string is REPLACED 
            // with a pointer to the array.
            
            matte_string_destroy(start->text);   // VERY SNEAKY
            start->text = (matteString_t *)inst; // VERY SNEAKIER
            

            // dispose of unneeded nodes since they were compiled.
            iter = start->next;
            matteToken_t * next;
            while(iter != end) {
                next = iter->next;
                destroy_token(iter);            
                iter = next;
            }
            start->next = end;
            break;
          
        }

    }
    
    iter = *src;
    
    // the idea is that an expression is a series of compute nodes
    // whose order of computation depends on its preceding operator

    while(iter->ttype != MATTE_TOKEN_MARKER_EXPRESSION_END) {
        int preOp = -1;
        int postOp = -1;
        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR1) {
            // operator first
            preOp = string_to_operator(opr->text);
            iter = iter->next;
        }


        // by this point, all complex sub-expressions would have been 
        // reduced, so now we can just work with raw values
        matteArray_t * valueInst = compile_value(g, block, functions, &iter);
        if (!valueInst) {
            goto L_FAIL;
        }

        // function calls are kind of like special operators
        // here, they use the compiled value above as the function object 
        // and have computed arguments passed to it
        if (iter->ttype == MATTE_TOKEN_FUNCTION_BEGIN) {
            assert(!"function calls still need to be implemented, sorry");
        }


        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR2) {
            // operator first
            postOp = string_to_operator(opr->text);
            iter = iter->next;
        }
        matteExpressionNode_t * exp = new_expression_node(
            preOp,
            postOp,
            valueInst
        );
        matte_array_push(nodes, exp);
    }


    // re-order based on precedence
    expression_node_sort(nodes);



    assert(!"then all the nodes are run in sorted order and are combined based on their operations and precedence");

    
    *src = iter->next; // skip marker!
    return outInst;
    
  L_FAIL:
    matte_array_destroy(nodes);
    matte_array_destroy(outInst);
    return NULL;
}





static int compile_statement(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    while(iter->ttype == MATTE_TOKEN_STATEMENT_END) {
        iter = iter->next;
    }
    if (!iter) {
        *src = NULL;
        return FALSE;
    }
    int varConst = 0;
    switch(iter->ttype) {
      // return statement
      case MATTE_TOKEN_RETURN: {
        iter = iter->next; // skip "return"
        uint32_t oln = iter->line;
        matteArray_t * inst;
        if (!(inst = compile_expression(g, block, functions, &iter))) {
            // error reporting handled by compile_expression
            return -1;
        }
        // when computed, should leave one value on the stack
        write_instructions(g, inst);
        write_instruction__ret(g, oln);
        iter = iter->next; // skip ;
        break;
      }

      case MATTE_TOKEN_WHEN: {
        uint32_t oln = iter->line;
        iter = iter->next; // skip when 
        iter = iter->next; // skip (
        matteArray_t * cond = compile_expression(g, block, functions, &iter);
        if (!cond) {
            return -1;
        }
        iter = iter->next; // skip )
        iter = iter->next; // skip :
        matteArray_t * ret;
        if (!(ret = compile_expression(g, block, functions, &iter))) {
            // error reporting handled by compile_expression
            return -1;            
        }
        
        write_instructions(g, cond);
        write_instruction__skp_insert(block->instructions, pc, oln, matte_array_get_size(ret));
        write_instructions(g, ret);    
        iter = iter->next; // skip ;
        break;
      }

      case MATTE_TOKEN_DECLARE_CONST:
        varConst = 1;
      case MATTE_TOKEN_DECLARE:
        iter = iter->next; // declaration
        if (varConst) {
            if (iter->ttype != MATTE_TOKEN_ASSIGNMENT &&
                iter->ttype != MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {
                matte_syntax_graph_print_compile_error(g, iter, "Constants must be explicitly set when declared.");
                return -1;
            }
        }

        // declaration only, no code
        if (iter->ttype == MATTE_TOKEN_STATEMENT_END) break;
        if (iter->ttype == MATTE_TOKEN_ASSIGNMENT) {
            iter = iter->next; // skip operator
        }
        matteArray_t * inst;
        if (!(inst = compile_expression(g, block, functions, &iter))) {
            // error reporting handled by compile_expression
            return -1;
        }
        write_instructions(g, inst);
        uint32_t g = get_local_referrable(g, block, iter);
        if (g != 0xffffffff) {
            write_instruction__arf(g, oln, g);
        } else {
            // bad referrable
            return -1;
        }
        break;


      default: {
        // when computed, should leave one value on the stack
        matteArray_t * inst;
        if (!(inst = compile_expression(g, block, functions, &iter))) {
            return -1;
        }
        write_instructions(g, inst);
        write_instruction__pop(g, oln, 1);
        iter = iter->next; // skip ;
      }
    }




    if (!iter) {
        *src = NULL;
        return FALSE;
    }

    while(iter->ttype == MATTE_TOKEN_STATEMENT_END) {
        iter = iter->next;
    }
    *src = iter;
    return 1;
}


static matteFunctionBlock_t * compile_function_block(
    matteSyntaxGraph_t * g, 
    matteFunctionBlock_t * parent,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteFunctionBlock_t * b = calloc(1, sizeof(matteFunctionBlock_t));
    b->args = matte_array_create(sizeof(matteString_t *));
    b->locals = matte_array_create(sizeof(matteString_t *));
    b->instructions = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    b->captures = matte_array_create(sizeof(matteBytecodeStubCapture_t));
    b->captureNames = matte_array_create(sizeof(matteString_t *));
    b->stubID = g->functionStubID++;
    b->parent = parent;

    #ifdef MATTE_DEBUG
        printf("COMPILING FUNCTION %d\n", b->stubID);
    #endif


    // gather all args first
    // a proper call to compile_function_block should start AT the '('
    // if present (err, after the function constructor more accurately)
    if (b->stubID != 0) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_BEGIN) {
            // args must ALWAYS be variable names 
            iter = iter->next;
            while(iter && iter->ttype == MATTE_TOKEN_VARIABLE_NAME) {
                matteString_t * arg = matte_string_clone(iter->text);
                matte_array_push(b->args, arg);
                #ifdef MATTE_DEBUG
                    printf("  - Argument %d: %s\n", matte_array_get_size(b->args), matte_string_get_c_str(arg));
                #endif
                iter = iter->next;
                if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_END) break;
                if (iter->ttype != MATTE_TOKEN_FUNCTION_ARG_SEPARATOR) {
                    matte_syntax_graph_print_compile_error(g, iter, "Expected ',' separator for arguments");
                    goto L_FAIL;                
                }
                iter = iter->next;
            }

            if (iter->ttype != MATTE_TOKEN_FUNCTION_ARG_END) {
                matte_syntax_graph_print_compile_error(g, iter, "Expected ')' to end function arguments");
                goto L_FAIL;
            }
            iter = iter->next;
        } 

        // most situations will require that the block begin '{' token 
        // exists already. This will be true EXCEPT in the toplevel function call (root stub)
        if (iter->ttype != MATTE_TOKEN_FUNCTION_BEGIN) {
            matte_syntax_graph_print_compile_error(g, iter, "Missing function block begin brace. '{'");
            goto L_FAIL;
        }
        iter = iter->next;
    }


    // next find all locals
    matteToken_t * funcStart = iter;
    while(iter && iter->ttype != MATTE_TOKEN_FUNCTION_END) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {
            ff_skip_inner_function(&iter);
        } else if (iter->ttype == MATTE_TOKEN_DECLARE ||
            iter->ttype == MATTE_TOKEN_DECLARE_CONST) {
            // TODO: const :(

            iter = iter->next;
            if (iter->ttype != MATTE_TOKEN_VARIABLE_NAME) {
                matte_syntax_graph_print_compile_error(g, iter, "Expected local variable name.");
                goto L_FAIL;
            }
            
            matteString_t * local = matte_string_clone(iter->text);
            matte_array_push(b->locals, local);
            #ifdef MATTE_DEBUG
                printf("  - Local %d: %s\n", matte_array_get_size(b->locals), matte_string_get_c_str(local));
            #endif
        }
        iter = iter->next;
    }

    // reset
    iter = funcStart;



    // now that we have all the locals and args, we can start emitting bytecode.
    int status;
    do {
        status = compile_statement(g, b, functions, &iter);
    } while(status == 1);

    if (status == -1) {
        // should be clean
        goto L_FAIL;
    }



    return b;
  L_FAIL:
    destroy_function_block(b);
    return NULL;
}

matteArray_t * matte_syntax_graph_compile(matteSyntaxGraph_t * g) {
    matteToken_t * iter = g->first;
    matteArray_t * arr = matte_array_create(sizeof(matteFunctionBlock_t *));
    matteFunctionBlock_t * root;
    if (!(root = compile_function_block(g, NULL, arr, &iter))) {
        return NULL;        
    }
    matte_array_push(arr, root);
    return arr;
}
