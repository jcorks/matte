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
    MATTE_TOKEN_FUNCTION_CONSTRUCTOR, // <=

    MATTE_TOKEN_WHEN,
    MATTE_TOKEN_WHEN_RETURN,
    MATTE_TOKEN_RETURN,

    MATTE_TOKEN_STATEMENT_END // newline OR ;
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
    MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,  
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, 
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, 
    MATTE_SYNTAX_CONSTRUCT_VALUE, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS 
};



// Creates a new syntax tree,
//
// A syntax graph allows reduction of raw, UT8 text 
// into parsible tokens. 
matteSyntaxGraph_t * matte_syntax_graph_create(
    matteTokenizer_t *,
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

// returns a new node that represents multiple valid choices 
// along the path. One NULL: ends a path, 2 NULLs: 
matteSyntaxGraphNode_t * matte_syntax_graph_node_split(
    matteSyntaxGraphNode_t *,
    ...
);

// Returns the first parsed token from the user source
matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraph_t * g);







uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch)
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraph_t * st = matte_syntax_graph_create(w, onError);
    matte_syntax_graph_register_constructs(
        st,
        MATTE_SYNTAX_CONSTRUCT_EXPRESSION, MATTE_STR_CAST("Expression"),
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, MATTE_STR_CAST("Function Call"),
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, MATTE_STR_CAST("Function Scope Statement"),
        MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT, MATTE_STR_CAST("New Object"),
        MATTE_SYNTAX_CONSTRUCT_VALUE, MATTE_STR_CAST("Value"),
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS, MATTE_STR_CAST("Function Call Argument List"),
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, MATTE_STR_CAST("New Function Argument List"),
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
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR, MATTE_STR_CAST("Function Constructor '<='"),

        MATTE_TOKEN_WHEN, MATTE_STR_CAST("'when' Statement"),
        MATTE_TOKEN_WHEN_RETURN, MATTE_STR_CAST("'when' Return Value Operator ':'"),
        MATTE_TOKEN_RETURN, MATTE_STR_CAST("'return' Statement"),

        MATTE_TOKEN_STATEMENT_END, MATTE_STR_CAST("Statement End ';' or newline"),
        NULL
    );


    ///////////////
    ///////////////
    /// VALUE
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
            matte_syntax_graph_node_end(),
            NULL,
            
            matte_syntax_graph_node_end(),
            NULL,

            NULL
        ),            
        NULL
    );


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_VARIABLE_NAME,
            MATTE_TOKEN_LITERAL_BOOLEAN,
            MATTE_TOKEN_LITERAL_NUMBER,
            MATTE_TOKEN_LITERAL_EMPTY,
            MATTE_TOKEN_LITERAL_STRING,
            0
        ),
        matte_syntax_graph_node_end(),
        NULL
    ); 

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT),
        matte_syntax_graph_node_end(),
        NULL
    ); 


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_construct(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_end(),            
            NULL,
            NULL
        ),
        NULL
    ); 






    ///////////////
    ///////////////
    /// Function Call
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),

        matte_syntax_graph_node_split(
            // no arg path
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
            NULL,

            // one or more args
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_split(
                // 1 arg
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(2), // back to arg expression
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
    /// Expression
    ///////////////
    ///////////////
    
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_to_parent(3),    
            NULL,
            matte_syntax_graph_node_end(),
            NULL,
            NULL
        ),
        NULL
    );

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR1),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_to_parent(3),    
            NULL,
            matte_syntax_graph_node_end(),
            NULL,
            NULL
        ),
        NULL,
        NULL 
    );
    
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXPRESSION_GROUP_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_EXPRESSION_GROUP_END),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
            matte_syntax_graph_node_end(),    
            NULL,
            matte_syntax_graph_node_end(),
            NULL,
            NULL
        ),
        NULL,
        NULL 
    );


    ///////////////
    ///////////////
    /// Function Creation Args
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS,
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
    /// New Object
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_END),
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_DEF_PROP),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_END),
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_STATIC_SEPARATOR),
                matte_syntax_graph_node_to_parent(2),
                NULL,
                NULL
            ),
            NULL,
            NULL
        ),
        NULL 
    );

    // function
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
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

    // array-like
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_START),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
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
    /// Function Scope Statement
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_WHEN),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_WHEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_DECLARE),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
            matte_syntax_graph_node_end(),
            NULL,
            NULL 
        ),
        NULL
    );

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_DECLARE_CONST),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );
    
    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
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

    matte_token_print(matte_syntax_graph_get_first(st));
    // then walk through the tokens and group them together.
    // a majority of compilation errors will likely happen here, 
    // as only a strict series of tokens are allowed in certain 
    // groups. Might want to form function groups with parenting to track referrables.
    
    // finally emit code for groups.
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



matteTokenizer_t * matte_tokenizer_create(const uint8_t * data, uint32_t byteCount) {
    matteTokenizer_t * t = calloc(1, sizeof(matteTokenizer_t));
    t->line = 1;
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
                t->backup = t->iter;
                t->character+=skipped;
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

          // newline is a valid statement end, so its not consumed here

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
    switch(ty) {
      case MATTE_TOKEN_LITERAL_NUMBER: {
        // convert into ascii
        int ccount = 0;
        int isDone = 0;
        matteString_t * out = matte_string_create();
        while(!isDone) {
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
                t->character++;
                t->backup = t->iter;
                break;
              default:
                t->iter = t->backup;
                isDone = 1;
                break;
            }
        }
        double f;
        if (sscanf(matte_string_get_c_str(out), "%f", &f) == 1) {
            return new_token(
                out, //xfer ownership
                currentLine,
                currentCh,
                ty
            );
        } else {
            matte_string_destroy(out);
            return NULL;
        }
        break;
      }            
    




      case MATTE_TOKEN_LITERAL_STRING: {
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case '\'':
          case '"':
            t->backup = t->iter;
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
                t->character++;
                return new_token(
                    text,
                    currentLine,
                    currentCh,
                    ty
                );
                break;
              default:
                t->iter = t->backup;
                matte_string_destroy(text);
                return NULL;            
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
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "<=");
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
          case '\n':
            t->line++;
            t->character = 1;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("\n"),
                currentLine,
                currentCh,
                ty
            );
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
int matte_tokenizer_is_end(const matteTokenizer_t * t){}


static void matte_token_print__helper(matteToken_t * t) {
    if (t == NULL) {
        printf("<token is NULL>\n");
        fflush(stdout);
        return;
    }
    printf("token:\n");
    printf("  line:  %d\n", t->line);
    printf("  ch:    %d\n", t->character);
    printf("  type:  %d\n", t->ttype);
    printf("  text:  '%s'\n", matte_string_get_c_str(t->text));
    fflush(stdout);


}

void matte_token_print(matteToken_t * t) {
    do {
        matte_token_print__helper(t);
        t = t->next;
    } while(t);

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

    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch);
};


matteSyntaxGraph_t * matte_syntax_graph_create(
    matteTokenizer_t * t,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch)
) {
    matteSyntaxGraph_t * out = calloc(1, sizeof(matteTokenizer_t));
    out->tokenizer = t;
    out->first = out->last = NULL;
    out->onError = onError;

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
                    printf("  - PASSED initial construct node for %s\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, node->construct)->name));
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
                    printf("  - FAILED initial construct node for %s\n", matte_string_get_c_str(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t *, node->construct)->name));
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
