#include "matte_compiler__syntax_graph.h"
#include "matte_string.h"
#include "matte_array.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

struct matteSyntaxGraph_t {
    // array of matteString_t *
    matteArray_t * tokenNames;
    // array of matteSYntaxGraphRoot_t *
    matteArray_t * constructRoots;
};








static void matte_syntax_graph_add_construct_path(
    matteSyntaxGraph_t *,
    const matteString_t * name, 
    int construct,
    ...
);

// returns a new node that represents a single token
static matteSyntaxGraphNode_t * matte_syntax_graph_node_token(

    int
);

// returns a new node that prepresents a possible set of 
// token types accepted
static matteSyntaxGraphNode_t * matte_syntax_graph_node_token_group(
    int,
    ...
);

// returns a new node that represents the end of a syntax 
// tree path.
static matteSyntaxGraphNode_t * matte_syntax_graph_node_end();


// returns a new node that represents a return to 
// a previous node. Level is how far up the tree this 
// previous node is. 0 is the self, 1 is the parent, etc
static matteSyntaxGraphNode_t * matte_syntax_graph_node_to_parent(int level);


// returns a new node that represents a construct sub-tree.
static matteSyntaxGraphNode_t * matte_syntax_graph_node_construct(int );

// returns a new node that represents a token marker.
// Token markers always succeed in the tree and produce a 
// token.
static matteSyntaxGraphNode_t * matte_syntax_graph_node_marker(int );


// returns a new node that represents multiple valid choices 
// along the path. One NULL: ends a path, 2 NULLs: 
static matteSyntaxGraphNode_t * matte_syntax_graph_node_split(
    matteSyntaxGraphNode_t *,
    ...
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
static void matte_syntax_graph_register_constructs(
    matteSyntaxGraph_t * g,
    int constructID, const matteString_t * name,
    ...
);


// For defining nodes of the syntax graph, the 
// nodes that accept 
static void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t *,
    int token, const matteString_t * name,
    ...
);



static void generate_graph(matteSyntaxGraph_t * g) {
    matte_syntax_graph_register_constructs(g,
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

    matte_syntax_graph_register_tokens(g, 
        // token is a number literal
        MATTE_TOKEN_LITERAL_NUMBER, MATTE_STR_CAST("Number Literal"),
        MATTE_TOKEN_LITERAL_STRING, MATTE_STR_CAST("String Literal"),
        MATTE_TOKEN_LITERAL_BOOLEAN, MATTE_STR_CAST("Boolean Literal"),
        MATTE_TOKEN_LITERAL_EMPTY, MATTE_STR_CAST("Empty Literal"),
        MATTE_TOKEN_EXTERNAL_NOOP, MATTE_STR_CAST("no-op built-in"),
        MATTE_TOKEN_EXTERNAL_GATE, MATTE_STR_CAST("Gate built-in"),
        MATTE_TOKEN_EXTERNAL_LOOP, MATTE_STR_CAST("While built-in"),
        MATTE_TOKEN_EXTERNAL_FOR, MATTE_STR_CAST("For built-in"),
        MATTE_TOKEN_EXTERNAL_FOREACH, MATTE_STR_CAST("Foreach built-in"),
        MATTE_TOKEN_EXTERNAL_MATCH, MATTE_STR_CAST("Match built-in"),
        MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION, MATTE_STR_CAST("Get External Function built-in"),
        MATTE_TOKEN_EXTERNAL_IMPORT, MATTE_STR_CAST("Import built-in"),
        MATTE_TOKEN_EXTERNAL_REMOVE_KEY, MATTE_STR_CAST("Remove Key built-in"),
        MATTE_TOKEN_EXTERNAL_TOSTRING, MATTE_STR_CAST("String cast built-in"),
        MATTE_TOKEN_EXTERNAL_TONUMBER, MATTE_STR_CAST("Number cast built-in"),
        MATTE_TOKEN_EXTERNAL_TOBOOLEAN, MATTE_STR_CAST("Boolean cast built-in"),
        MATTE_TOKEN_EXTERNAL_INTROSPECT, MATTE_STR_CAST("Introspect built-in"),
        MATTE_TOKEN_EXTERNAL_PRINT, MATTE_STR_CAST("Print built-in"),
        MATTE_TOKEN_EXTERNAL_ERROR, MATTE_STR_CAST("Error built-in"),
        MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, MATTE_STR_CAST("Expression '('"),
        MATTE_TOKEN_EXPRESSION_GROUP_END, MATTE_STR_CAST("Expression ')'"),
        MATTE_TOKEN_ASSIGNMENT, MATTE_STR_CAST("Assignment Operator '='"),
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START, MATTE_STR_CAST("Object Accessor '['"),
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END, MATTE_STR_CAST("Object Accessor ']'"),
        MATTE_TOKEN_OBJECT_ACCESSOR_DOT, MATTE_STR_CAST("Object Accessor '.'"),
        MATTE_TOKEN_GENERAL_OPERATOR1, MATTE_STR_CAST("Single-operand Operator"),
        MATTE_TOKEN_GENERAL_OPERATOR2, MATTE_STR_CAST("Operator"),
        MATTE_TOKEN_VARIABLE_NAME, MATTE_STR_CAST("Variable Name"),
        MATTE_TOKEN_OBJECT_LITERAL_BEGIN, MATTE_STR_CAST("Object Literal '{'"),
        MATTE_TOKEN_OBJECT_LITERAL_END, MATTE_STR_CAST("Object Literal '}'"),
        MATTE_TOKEN_OBJECT_DEF_PROP, MATTE_STR_CAST("Object Literal assignment ':'"),
        MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR, MATTE_STR_CAST("Object Literal separator ','"),
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
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR, MATTE_STR_CAST("Function Constructor '<-'"),

        MATTE_TOKEN_WHEN, MATTE_STR_CAST("'when' Statement"),
        MATTE_TOKEN_GATE_RETURN, MATTE_STR_CAST("'gate' Else Operator"),
        MATTE_TOKEN_MATCH_BEGIN, MATTE_STR_CAST("'match' Content Block '{'"),
        MATTE_TOKEN_MATCH_END, MATTE_STR_CAST("'match' Content Block '}'"),
        MATTE_TOKEN_MATCH_IMPLIES, MATTE_STR_CAST("'match' implies ':'"),
        MATTE_TOKEN_MATCH_SEPARATOR, MATTE_STR_CAST("'match' separator ','"),
        MATTE_TOKEN_MATCH_DEFAULT, MATTE_STR_CAST("'match' default ','"),

        MATTE_TOKEN_RETURN, MATTE_STR_CAST("'return' Statement"),

        MATTE_TOKEN_STATEMENT_END, MATTE_STR_CAST("Statement End ';'"),
        MATTE_TOKEN_MARKER_EXPRESSION_END, MATTE_STR_CAST("[marker]"),

        NULL
    );


    ///////////////
    ///////////////
    /// VALUE
    ///////////////
    ///////////////



    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Variable"), MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_end(),
        NULL
    );




    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Object Constructor"),  MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT),
        matte_syntax_graph_node_end(),
        NULL
    ); 






    ///////////////
    ///////////////
    /// Function Call
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL,
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
                matte_syntax_graph_node_to_parent(4), // back to arg expression
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
    


    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("2-Operand"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Dot Accessor + Assignment"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_DOT),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_to_parent(2),    
            NULL,

            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_end(),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL
    );

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Bracket Accessor + Assignment"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_to_parent(2),    
            NULL,

            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_end(),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL
    );



    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Function Call"), MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
        matte_syntax_graph_node_end(),    
        NULL
    );




    ///////////////
    ///////////////
    /// Expression
    ///////////////
    ///////////////




    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Prefix Operator"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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
    
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Expression Enclosure"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXPRESSION_GROUP_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
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

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Gate Expression"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXTERNAL_GATE),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_START),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_END),

        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),

        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_GATE_RETURN),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),    
            NULL,


            matte_syntax_graph_node_end(),    
            NULL,

            NULL
        ),
        NULL
    );

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Match Expression"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXTERNAL_MATCH),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_START),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_END),

        matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_BEGIN),        

        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_START),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_END),
                matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_IMPLIES),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),

                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_SEPARATOR),
                    matte_syntax_graph_node_to_parent(11), // back to function arg begin split, hopefully i counted right
                    NULL,


                    matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_END),
                    matte_syntax_graph_node_end(),    
                    NULL,
                    NULL
                ),
                NULL,

                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(4), // back to arg expression
                NULL,
                NULL
            ),
            NULL,


            matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_DEFAULT),
            matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_IMPLIES),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),

            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_SEPARATOR),
                matte_syntax_graph_node_to_parent(7), // back to function arg begin split, hopefully i counted right
                NULL,


                matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_END),
                matte_syntax_graph_node_end(),    
                NULL,
                NULL
            ),
            NULL,


            NULL
        ),
        NULL

    );


    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Literal Value"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_LITERAL_BOOLEAN,
            MATTE_TOKEN_LITERAL_NUMBER,
            MATTE_TOKEN_LITERAL_EMPTY,
            MATTE_TOKEN_LITERAL_STRING,

            MATTE_TOKEN_EXTERNAL_NOOP,
            MATTE_TOKEN_EXTERNAL_LOOP,
            MATTE_TOKEN_EXTERNAL_FOREACH,
            MATTE_TOKEN_EXTERNAL_FOR,
            MATTE_TOKEN_EXTERNAL_MATCH,
            MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION,
            MATTE_TOKEN_EXTERNAL_IMPORT,
            MATTE_TOKEN_EXTERNAL_REMOVE_KEY,
            MATTE_TOKEN_EXTERNAL_TOSTRING,
            MATTE_TOKEN_EXTERNAL_TONUMBER,
            MATTE_TOKEN_EXTERNAL_TOBOOLEAN,
            MATTE_TOKEN_EXTERNAL_INTROSPECT,
            MATTE_TOKEN_EXTERNAL_PRINT,
            MATTE_TOKEN_EXTERNAL_ERROR,

            0
        ),
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

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Variable Access / Assignment"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(

            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
                matte_syntax_graph_node_to_parent(2),    
                NULL,

                matte_syntax_graph_node_end(),    
                NULL,

                NULL
            ),
            NULL,



            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),    
            NULL,

            matte_syntax_graph_node_end(),    
            NULL,

            NULL

        ),
        NULL
    );


    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Simple Value"), MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS,
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
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Object Literal Constructor"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
                    matte_syntax_graph_node_end(),
                    NULL,
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR),
                    matte_syntax_graph_node_to_parent(7),
                    NULL,
                    NULL
                ),
                NULL,
                


                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_DEF_PROP),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
                    matte_syntax_graph_node_end(),
                    NULL,
                    matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR),
                    matte_syntax_graph_node_to_parent(8),
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


    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Function"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
        matte_syntax_graph_node_end(),
        NULL
    );


    // array-like
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Array-like Literal"), MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
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
    /// New Object: function constructor
    ///////////////
    ///////////////

    // function
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST(""), MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION,
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

    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Return Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("When Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_WHEN),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );
    



    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Declaration + Assignment"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_DECLARE,
            MATTE_TOKEN_DECLARE_CONST,
            0
        ),
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
            matte_syntax_graph_node_end(),
            NULL,


            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
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

    
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Expression"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    
    matte_syntax_graph_add_construct_path(g, MATTE_STR_CAST("Empty Statement"), MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );
}

matteSyntaxGraph_t * matte_syntax_graph_create() {
    matteSyntaxGraph_t * out = calloc(1, sizeof(matteSyntaxGraph_t));
    out->tokenNames = matte_array_create(sizeof(matteString_t *));
    out->constructRoots = matte_array_create(sizeof(matteSyntaxGraphRoot_t *));
    generate_graph(out);
    return out;
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
    n->token.marker = 0;
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
    n->token.marker = 1;
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

matteSyntaxGraphRoot_t * matte_syntax_graph_get_root(
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
    matte_string_set(matte_array_at(g->tokenNames, matteString_t *, token), name);

    va_list args;
    va_start(args, name);
    for(;;) {
        int id = va_arg(args, int);
        if (id == 0) break;

        const matteString_t * str = va_arg(args, matteString_t *);
        matte_syntax_graph_set_token_count(g, id);
        matte_string_set(matte_array_at(g->tokenNames, matteString_t *, id), str);
    }
    va_end(args);
}



const matteString_t * matte_syntax_graph_get_token_name(matteSyntaxGraph_t * g, int en) {
    return matte_array_at(g->tokenNames, matteString_t *, en);
}

int matte_syntax_graph_is_construct(matteSyntaxGraph_t * g, int constructID) {
    return constructID >= 0 && constructID < matte_array_get_size(g->constructRoots);
}