#include "matte_compiler.h"
#include "matte_array.h"
#include "matte_string.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>





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



typedef struct {
    // type of the token
    matteTokenType_t ttype;
    
    // source line number
    uint32_t line;
    // char within the line where it starts.

    uint32_t character;
    // text for the token
    matteString_t * text;
} matteToken_t;



// Tokenizer attempts to 
typedef struct matteTokenizer_t matteTokenizer_t;


// Creates a new tokenizer.
matteTokenizer_t * matte_tokenizer_create(const uint8_t * data, uint32_t len);

// Destroys the tokenizer and all associated matteToken_t * instances.
void matte_tokenizer_destroy(matteTokenizer_t *);

// returns the current line of the tokenizer.
// The initial line is 1.
uint32_t matte_tokenizer_current_line(const matteTokenizer_t *);

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
    matteArray_t * tokens,
    void (*onError)(const uint8_t * errMessage, int line)
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
    int constructID, const uint8_t * name,
    ...
);


// For defining nodes of the syntax graph, the 
// nodes that accept 
void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t *,
    int token, const uint8_t * name,
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










uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const char *, uint32_t)
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);


    ////////// TESTING
    {
        matte_token_print(matte_tokenizer_next(w, MATTE_TOKEN_RETURN));   
        matte_token_print(matte_tokenizer_next(w, MATTE_TOKEN_VARIABLE_NAME));   
        matte_token_print(matte_tokenizer_next(w, MATTE_TOKEN_GENERAL_OPERATOR2));   
        matte_token_print(matte_tokenizer_next(w, MATTE_TOKEN_LITERAL_NUMBER));   
        matte_token_print(matte_tokenizer_next(w, MATTE_TOKEN_STATEMENT_END));   
        exit(1);
    }



    matteArray_t * arr = matte_array_create(sizeof(matteToken_t *));
    matteSyntaxGraph_t * st = matte_syntax_graph_create(w, arr, onError);
    matte_syntax_graph_register_constructs(
        st,
        MATTE_SYNTAX_CONSTRUCT_EXPRESSION, "Expression",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, "Function Call",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, "Function Scope Statement",
        MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT, "New Object",
        MATTE_SYNTAX_CONSTRUCT_VALUE, "Value",
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS, "Function Call Argument List",
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, "New Function Argument List",
        NULL
    );

    matte_syntax_graph_register_tokens(
        st,
        // token is a number literal
        MATTE_TOKEN_LITERAL_NUMBER, "Number Literal",
        MATTE_TOKEN_LITERAL_STRING, "String Literal",
        MATTE_TOKEN_LITERAL_BOOLEAN, "Boolean Literal",
        MATTE_TOKEN_LITERAL_EMPTY, "Empty Literal",
        MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, "Expression '('",
        MATTE_TOKEN_EXPRESSION_GROUP_END, "Expression ')'",
        MATTE_TOKEN_ASSIGNMENT, "Assignment Operator '='",
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START, "Object Accessor '['",
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END, "Object Accessor ']'",
        MATTE_TOKEN_OBJECT_ACCESSOR_DOT, "Object Accessor '.'",
        MATTE_TOKEN_GENERAL_OPERATOR1, "Single-operand Operator",
        MATTE_TOKEN_GENERAL_OPERATOR2, "Operator",
        MATTE_TOKEN_VARIABLE_NAME, "Variable Name:",
        MATTE_TOKEN_OBJECT_STATIC_BEGIN, "Object Literal '{'",
        MATTE_TOKEN_OBJECT_STATIC_END, "Object Literal '}'",
        MATTE_TOKEN_OBJECT_DEF_PROP, "Object Literal assignment ':'",
        MATTE_TOKEN_OBJECT_STATIC_SEPARATOR, "Object Literal separator ','",
        MATTE_TOKEN_OBJECT_ARRAY_START, "Array Literal '['",
        MATTE_TOKEN_OBJECT_ARRAY_END, "Array Literal ']'",
        MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR, "Array Element Separator ','",

        MATTE_TOKEN_DECLARE, "Local Variable Declarator '@'",
        MATTE_TOKEN_DECLARE_CONST, "Local Constant Declarator '<@>'",

        MATTE_TOKEN_FUNCTION_BEGIN, "Function Constent Block '{'",
        MATTE_TOKEN_FUNCTION_END,   "Function Constent Block '}'",
        MATTE_TOKEN_FUNCTION_ARG_BEGIN, "Function Argument List '('",
        MATTE_TOKEN_FUNCTION_ARG_SEPARATOR, "Function Argument Separator ','",
        MATTE_TOKEN_FUNCTION_ARG_END, "Function Argument List ')'",
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR, "Function Constructor '<='",

        MATTE_TOKEN_WHEN, "'when' Statement",
        MATTE_TOKEN_WHEN_RETURN, "'when' Return Value Operator ':'",
        MATTE_TOKEN_RETURN, "'return' Statement",

        MATTE_TOKEN_STATEMENT_END, "Statement End ';' or newline",
        NULL
    );


    ///////////////
    ///////////////
    /// VALUE
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_VARIABLE_NAME,
            MATTE_TOKEN_LITERAL_BOOLEAN,
            MATTE_TOKEN_LITERAL_NUMBER,
            MATTE_TOKEN_LITERAL_EMPTY,
            MATTE_TOKEN_LITERAL_STRING
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


    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
        matte_syntax_graph_node_end(),
        NULL
    );




    ///////////////
    ///////////////
    /// Function Call
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
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
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_to_parent(1),    
                NULL,
                matte_syntax_graph_node_end(),
                NULL, 
                NULL                
            ),
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
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_to_parent(1),    
                NULL,
                matte_syntax_graph_node_end(),
                NULL, 
                NULL                
            ),
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
            matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_to_parent(1),    
                NULL,
                matte_syntax_graph_node_end(),
                NULL, 
                NULL                
            ),
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
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_to_parent(1),
                NULL,

                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_END),
                matte_syntax_graph_node_end(),
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
        matte_syntax_graph_node_token(MATTE_TOKEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );


   
    // use the text walker to generate an array of tokens for 
    // the entire source. Basic case here allows you to 
    // filter out any poorly formed contextual tokens
    int error;
    while(!(error = matte_syntax_graph_continue(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT)));
    if (error) {
        *size = 0;
        return NULL;
    }
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

// skips space and newlines
int matte_tokenizer_peek_next(matteTokenizer_t * t) {
    uint8_t * iterC = t->iter;  
    return utf8_next_char(&iterC);
}


static void tokenizer_strip(matteTokenizer_t * t) {
    int c;
    for(;;) {
        c = utf8_next_char(&t->iter);
        t->character++;
        switch(c) {
          case '\t':
          case '\r':
          case ' ':
          case '\v':
          case '\f':
            t->backup = t->iter;
            break;

          // newline is a valid statement end, so its not consumed here

          // includes 0
          default:;
            t->iter = t->backup;
            return;
        }
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
        if (utf8_next_char(&t->iter) == 'e' &&
            utf8_next_char(&t->iter) == 'm' &&
            utf8_next_char(&t->iter) == 'p' &&
            utf8_next_char(&t->iter) == 't' &&
            utf8_next_char(&t->iter) == 'y'
        ) {
            t->character += 5;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("%s", "empty"),
                currentLine,
                currentCh,
                ty
            );

        } else {
            t->iter = t->backup;
            return NULL;
        }     
        break;
      }

      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: {
        int c = utf8_next_char(&t->iter);
        if (c == '(') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("("),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      } // (
      case MATTE_TOKEN_EXPRESSION_GROUP_END: {
        int c = utf8_next_char(&t->iter);
        if (c == ')') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(")"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;

      } // )
      case MATTE_TOKEN_ASSIGNMENT: {
        int c = utf8_next_char(&t->iter);
        if (c == '=') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("="),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START: {
        int c = utf8_next_char(&t->iter);
        if (c == '[') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("["),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;

      }

      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END: {
        int c = utf8_next_char(&t->iter);
        if (c == ']') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("]"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }
      case MATTE_TOKEN_OBJECT_ACCESSOR_DOT: {
        int c = utf8_next_char(&t->iter);
        if (c == '.') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("."),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
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
        int c = utf8_next_char(&t->iter);
        if (c == '{') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("{"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }
      case MATTE_TOKEN_OBJECT_STATIC_END: {
        int c = utf8_next_char(&t->iter);
        if (c == '}') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("}"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }

      case MATTE_TOKEN_OBJECT_DEF_PROP: {
        int c = utf8_next_char(&t->iter);
        if (c == ':') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(":"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }

      case MATTE_TOKEN_OBJECT_STATIC_SEPARATOR: {
        int c = utf8_next_char(&t->iter);
        if (c == ',') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(","),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_START: {
        int c = utf8_next_char(&t->iter);
        if (c == '[') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("["),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_END: {
        int c = utf8_next_char(&t->iter);
        if (c == ']') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("]"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }
      case MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR: {
        int c = utf8_next_char(&t->iter);
        if (c == ',') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(","),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }

      case MATTE_TOKEN_DECLARE: {
        int c = utf8_next_char(&t->iter);
        if (c == '@') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("@"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_DECLARE_CONST: {
        if (utf8_next_char(&t->iter) == '<' &&
            utf8_next_char(&t->iter) == '@' &&
            utf8_next_char(&t->iter) == '>'
        ) {
            t->character += 3;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("<@>"),
                currentLine,
                currentCh,
                ty
            );

        } else {
            t->iter = t->backup;
            return NULL;
        }     
        break;          
      }

      case MATTE_TOKEN_FUNCTION_BEGIN: {
        int c = utf8_next_char(&t->iter);
        if (c == '{') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("{"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_FUNCTION_END: {
        int c = utf8_next_char(&t->iter);
        if (c == '}') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("}"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_BEGIN: {
        int c = utf8_next_char(&t->iter);
        if (c == '(') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("("),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_SEPARATOR: {
        int c = utf8_next_char(&t->iter);
        if (c == ',') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(","),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_END: {
        int c = utf8_next_char(&t->iter);
        if (c == ')') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(")"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break;          
      }
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {
        if (utf8_next_char(&t->iter) == '<' &&
            utf8_next_char(&t->iter) == '=' 
        ) {
            t->character += 2;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("<="),
                currentLine,
                currentCh,
                ty
            );

        } else {
            t->iter = t->backup;
            return NULL;
        }     
        break;  
      }

      case MATTE_TOKEN_WHEN: {
        if (utf8_next_char(&t->iter) == 'w' &&
            utf8_next_char(&t->iter) == 'h' &&
            utf8_next_char(&t->iter) == 'e' &&
            utf8_next_char(&t->iter) == 'n' 
        ) {
            t->character += 4;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("when"),
                currentLine,
                currentCh,
                ty
            );

        } else {
            t->iter = t->backup;
            return NULL;
        }     
        break;            
      }
      case MATTE_TOKEN_WHEN_RETURN: {
        int c = utf8_next_char(&t->iter);
        if (c == ':') {
            t->character++;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(":"),
                currentLine,
                currentCh,
                ty
            );            
        } else {
            t->iter = t->backup;
            return NULL;
        }
        break; 
      }
        
      case MATTE_TOKEN_RETURN: {
        if (utf8_next_char(&t->iter) == 'r' &&
            utf8_next_char(&t->iter) == 'e' &&
            utf8_next_char(&t->iter) == 't' &&
            utf8_next_char(&t->iter) == 'u' &&
            utf8_next_char(&t->iter) == 'r' &&
            utf8_next_char(&t->iter) == 'n'
        ) {
            t->character += 6;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str("%s", "return"),
                currentLine,
                currentCh,
                ty
            );

        } else {
            t->iter = t->backup;
            return NULL;
        }     
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

void matte_token_print(matteToken_t * t) {
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
















//////////////////////////////////////////
// syntax graph 

matteSyntaxGraph_t * matte_syntax_graph_create(
    matteTokenizer_t * t,
    matteArray_t * tokens,
    void (*onError)(const uint8_t * errMessage, int line)
){}


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
    matteSyntaxGraph_t * t,
    int constructID, const uint8_t * name,
    ...
){}


// For defining nodes of the syntax graph, the 
// nodes that accept 
void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t * g,
    int token, const uint8_t * name,
    ...
){}


int matte_syntax_graph_continue(
    matteSyntaxGraph_t * graph,
    int constructID
){}



void matte_syntax_graph_add_construct_path(
    matteSyntaxGraph_t * graph,
    ...
){}

// returns a new node that represents a single token
matteSyntaxGraphNode_t * matte_syntax_graph_node_token(
    int a
){}

// returns a new node that prepresents a possible set of 
// token types accepted
matteSyntaxGraphNode_t * matte_syntax_graph_node_token_group(
    int g,
    ...
){}

// returns a new node that represents the end of a syntax 
// tree path.
matteSyntaxGraphNode_t * matte_syntax_graph_node_end(){}


// returns a new node that represents a return to 
// a previous node. Level is how far up the tree this 
// previous node is. 0 is the self, 1 is the parent, etc
matteSyntaxGraphNode_t * matte_syntax_graph_node_to_parent(int level){}


// returns a new node that represents a construct sub-tree.
matteSyntaxGraphNode_t * matte_syntax_graph_node_construct(int a){}

// returns a new node that represents multiple valid choices 
// along the path. One NULL: ends a path, 2 NULLs: 
matteSyntaxGraphNode_t * matte_syntax_graph_node_split(
    matteSyntaxGraphNode_t * n,
    ...
){}