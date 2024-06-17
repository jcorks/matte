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
#include "matte_compiler__syntax_graph.h"
#include "matte_string.h"
#include "matte_array.h"
#include "matte.h"
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
    const char * name, 
    int construct,
    ...
);

// returns a new node that represents a single token
static matteSyntaxGraphNode_t * matte_syntax_graph_node_token(

    int
);

// returns a new node that represents a single token, but 
// when compiled, the first token is changed to the second token.
static matteSyntaxGraphNode_t * matte_syntax_graph_node_token_alias(
    int fromRequired,
    int to
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
    int constructID, const char * name,
    ...
);


// For defining nodes of the syntax graph, the 
// nodes that accept 
static void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t *,
    int token, const char * name,
    ...
);



static void generate_graph(matteSyntaxGraph_t * g) {
    matte_syntax_graph_register_constructs(g,
        MATTE_SYNTAX_CONSTRUCT_EXPRESSION, "Expression",
        MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION, "New Function",
        MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION_WITH_SPECIFIER, "New Function (with specifier)",
        MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT, "New Object",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, "Function Call",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, "Function Scope Statement",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_BODY, "Function Body",
        MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING, "Value Binding",
        MATTE_SYNTAX_CONSTRUCT_VALUE, "Value",
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, "New Function Argument List",
        MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS, "Function Call Argument List",
        MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION, "Function Definition",
        MATTE_SYNTAX_CONSTRUCT_POSTFIX, "Postfix",
        MATTE_SYNTAX_CONSTRUCT_MATCH_IMPLICATION, "Match Implication",
        NULL
    );

    matte_syntax_graph_register_tokens(g, 
        // token is a number literal
        MATTE_TOKEN_LITERAL_NUMBER, "Number Literal",
        MATTE_TOKEN_LITERAL_STRING, "String Literal",
        MATTE_TOKEN_LITERAL_BOOLEAN, "Boolean Literal",
        MATTE_TOKEN_LITERAL_EMPTY, "Empty Literal",
        MATTE_TOKEN_EXTERNAL_NOOP, "no-op built-in",
        MATTE_TOKEN_EXTERNAL_GATE, "Gate built-in",
        MATTE_TOKEN_EXTERNAL_MATCH, "Match built-in",
        MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION, "Get External Function built-in",
        MATTE_TOKEN_EXTERNAL_IMPORT, "Import built-in",

        MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN, "Boolean Type object",
        MATTE_TOKEN_EXTERNAL_TYPEEMPTY, "Empty Type object",
        MATTE_TOKEN_EXTERNAL_TYPENUMBER, "Number Type object",
        MATTE_TOKEN_EXTERNAL_TYPESTRING, "String Type object",
        MATTE_TOKEN_EXTERNAL_TYPEOBJECT, "Object Type object",
        MATTE_TOKEN_EXTERNAL_TYPETYPE, "Type Type object",
        MATTE_TOKEN_EXTERNAL_TYPEFUNCTION, "Function Type object",
        MATTE_TOKEN_EXTERNAL_TYPEANY, "Any Type object",
        MATTE_TOKEN_EXTERNAL_TYPENULLABLE, "Nullable Type object",


        MATTE_TOKEN_EXTERNAL_PRINT, "Print built-in",
        MATTE_TOKEN_EXTERNAL_SEND, "Send built-in",
        MATTE_TOKEN_EXTERNAL_ERROR, "Error built-in",
        MATTE_TOKEN_EXTERNAL_BREAKPOINT, "Breakpoint built-in",
        MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, "Expression '('",
        MATTE_TOKEN_EXPRESSION_GROUP_END, "Expression ')'",
        MATTE_TOKEN_ASSIGNMENT, "Assignment Operator '='",
        MATTE_TOKEN_ASSIGNMENT_ADD, "Addition Assignment'+='",
        MATTE_TOKEN_ASSIGNMENT_SUB, "Subtraction Assignment '-='",
        MATTE_TOKEN_ASSIGNMENT_MULT, "Multiplication Assignment '*='",
        MATTE_TOKEN_ASSIGNMENT_DIV, "Division Assignment '/='",
        MATTE_TOKEN_ASSIGNMENT_MOD, "Modulo Assignment '%='",
        MATTE_TOKEN_ASSIGNMENT_AND, "AND Assignment '&='",
        MATTE_TOKEN_ASSIGNMENT_OR, "OR Assignment '|='",
        MATTE_TOKEN_ASSIGNMENT_XOR, "XOR Assignment '^='",
        MATTE_TOKEN_ASSIGNMENT_BLEFT, "Left-Shift Assignment '<<='",
        MATTE_TOKEN_ASSIGNMENT_BRIGHT, "Right-Shift Assignment '>>='",
        MATTE_TOKEN_ASSIGNMENT_POW, "Power Assignment '**='",

        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START, "Object Accessor '['",
        MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END, "Object Accessor ']'",
        MATTE_TOKEN_OBJECT_ACCESSOR_DOT, "Object Accessor '.'",
        MATTE_TOKEN_GENERAL_OPERATOR1, "Single-operand Operator",
        MATTE_TOKEN_GENERAL_OPERATOR2, "Operator",
        MATTE_TOKEN_VARIABLE_NAME, "Variable Name Label",
        MATTE_TOKEN_OBJECT_LITERAL_BEGIN, "Object Literal '{'",
        MATTE_TOKEN_OBJECT_LITERAL_END, "Object Literal '}'",
        MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR, "Object Literal separator ','",
        MATTE_TOKEN_OBJECT_ARRAY_START, "Array Literal '['",
        MATTE_TOKEN_OBJECT_ARRAY_END, "Array Literal ']'",
        MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR, "Array Element Separator ','",
        MATTE_TOKEN_OBJECT_SPREAD, "Object Spread Operator '...'",

        MATTE_TOKEN_DECLARE, "Local Variable Declarator '@'",
        MATTE_TOKEN_DECLARE_CONST, "Local Constant Declarator '@:'",


        MATTE_TOKEN_FUNCTION_BEGIN, "Function Content Block '{'",
        MATTE_TOKEN_FUNCTION_END,   "Function Content Block '}'",
        MATTE_TOKEN_FUNCTION_ARG_BEGIN, "Function Argument List '('",
        MATTE_TOKEN_FUNCTION_ARG_SEPARATOR, "Function Argument Separator ','",
        MATTE_TOKEN_FUNCTION_ARG_END, "Function Argument List ')'",
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR, "Function Constructor '::'",
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR_INLINE, "Function Constructor Inline '<-'",
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR_WITH_SPECIFIER, "Function Constructor with Specifier ':::'",
        MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH, "Function Constructor Dash'<='",
        MATTE_TOKEN_FUNCTION_TYPESPEC, "Function Type Specifier '=>'",
        MATTE_TOKEN_FUNCTION_LISTEN, "Listen Expression Start '[::]'",

        MATTE_TOKEN_WHEN, "'when' Statement",
        MATTE_TOKEN_FOR, "'for' Statement",
        MATTE_TOKEN_FOREVER, "'forever' Statement",
        MATTE_TOKEN_FOR_SEPARATOR, "':' for statement separator",
        MATTE_TOKEN_FOREVER, "'<=' loop implicator",
        MATTE_TOKEN_GATE_RETURN, "'gate' Else Operator",
        MATTE_TOKEN_MATCH_BEGIN, "'match' Content Block '{'",
        MATTE_TOKEN_MATCH_END, "'match' Content Block '}'",
        MATTE_TOKEN_MATCH_SEPARATOR, "'match' separator ','",
        MATTE_TOKEN_IMPLICATION_START, "'match' implication start",
        MATTE_TOKEN_IMPLICATION_END, "'match' implication end",
        MATTE_TOKEN_MATCH_DEFAULT, "'match' default ','",

        MATTE_TOKEN_RETURN, "'return' Statement",

        MATTE_TOKEN_STATEMENT_END, "Statement End ';'",
        MATTE_TOKEN_MARKER_EXPRESSION_END, "[marker]",
        MATTE_TOKEN_GENERAL_SPECIFIER, "Specifier ':'",

        NULL
    );


    ///////////////
    ///////////////
    /// VALUE
    ///////////////
    ///////////////



    matte_syntax_graph_add_construct_path(g, "Variable", MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_end(),
        NULL
    );




    matte_syntax_graph_add_construct_path(g, "Object Constructor",  MATTE_SYNTAX_CONSTRUCT_VALUE,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT),
        matte_syntax_graph_node_end(),
        NULL
    ); 



    ///////////////
    ///////////////
    /// "Value Binding" construct (`: [expression]` and its variants
    /// used for function calls, object constructors, and match implications
    /// 
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(g, "Value Binding Expression", MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING,
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION_WITH_SPECIFIER),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),
            NULL,

            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),
            NULL,
                            
            matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_SPECIFIER),                
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
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

    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),

        matte_syntax_graph_node_split(
            // no arg path
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
            NULL,
            
            // vararg call 
            matte_syntax_graph_node_token(MATTE_TOKEN_VARARG),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),                
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
            NULL,    
            

                    

            // one or more args
            matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(3), // back to arg expression
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING),
                matte_syntax_graph_node_split(
                    // 1 arg
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
                    matte_syntax_graph_node_end(),
                    NULL,
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                    matte_syntax_graph_node_to_parent(5), // back to arg expression
                    NULL,
                    NULL
                ),
                NULL,                    
                
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
                matte_syntax_graph_node_end(),
                NULL,

                NULL
            ),
            NULL,
            // single argument binding (auto-binding)
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING),
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
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
    

    

    matte_syntax_graph_add_construct_path(g, "Query Operator", MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_QUERY_OPERATOR),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );


    matte_syntax_graph_add_construct_path(g, "2-Operand", MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_OPERATOR2),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_end(),    
        NULL
    );

    matte_syntax_graph_add_construct_path(g, "Dot Accessor + Assignment", MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_DOT),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_ASSIGNMENT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),    
            NULL,
            
            matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
            matte_syntax_graph_node_split(

                matte_syntax_graph_node_token_group(
                    MATTE_TOKEN_ASSIGNMENT_POW,
                    MATTE_TOKEN_ASSIGNMENT_ADD,
                    MATTE_TOKEN_ASSIGNMENT_SUB,
                    MATTE_TOKEN_ASSIGNMENT_MULT,
                    MATTE_TOKEN_ASSIGNMENT_DIV,
                    MATTE_TOKEN_ASSIGNMENT_MOD,
                    MATTE_TOKEN_ASSIGNMENT_AND,
                    MATTE_TOKEN_ASSIGNMENT_OR,
                    MATTE_TOKEN_ASSIGNMENT_XOR,
                    MATTE_TOKEN_ASSIGNMENT_BLEFT,
                    MATTE_TOKEN_ASSIGNMENT_BRIGHT,
                    0
                ),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
                matte_syntax_graph_node_end(),    
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_POSTFIX),
                matte_syntax_graph_node_to_parent(2),    
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
            NULL,
            NULL
        ),
        NULL
    );

    matte_syntax_graph_add_construct_path(g, "Bracket Accessor + Assignment", MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token_group(
                MATTE_TOKEN_ASSIGNMENT_POW,
                MATTE_TOKEN_ASSIGNMENT_ADD,
                MATTE_TOKEN_ASSIGNMENT_SUB,
                MATTE_TOKEN_ASSIGNMENT_MULT,
                MATTE_TOKEN_ASSIGNMENT_DIV,
                MATTE_TOKEN_ASSIGNMENT_MOD,
                MATTE_TOKEN_ASSIGNMENT_AND,
                MATTE_TOKEN_ASSIGNMENT_OR,
                MATTE_TOKEN_ASSIGNMENT_XOR,
                MATTE_TOKEN_ASSIGNMENT_BLEFT,
                MATTE_TOKEN_ASSIGNMENT_BRIGHT,
                0
            ),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),    
            NULL,

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



    matte_syntax_graph_add_construct_path(g, "Function Call", MATTE_SYNTAX_CONSTRUCT_POSTFIX,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL),
        matte_syntax_graph_node_end(),    
        NULL
    );




    ///////////////
    ///////////////
    /// Expression
    ///////////////
    ///////////////


    
    matte_syntax_graph_add_construct_path(g, "Listen Function", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_LISTEN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION),                

        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_SPECIFIER),                
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


    matte_syntax_graph_add_construct_path(g, "Prefix Operator", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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
    
    matte_syntax_graph_add_construct_path(g, "Expression Enclosure", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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

    matte_syntax_graph_add_construct_path(g, "Gate Expression", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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
    matte_syntax_graph_add_construct_path(g, "Match Implication", MATTE_SYNTAX_CONSTRUCT_MATCH_IMPLICATION,
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_START),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),   
            matte_syntax_graph_node_split(         
                matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_END),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING),            
                matte_syntax_graph_node_end(),
                NULL,
                
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(4),
                NULL,
                
                NULL 
            ),
                    

            NULL,
            matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_DEFAULT),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING),
            matte_syntax_graph_node_end(),    
            NULL,
            NULL
            
        ),
        NULL

    );

    matte_syntax_graph_add_construct_path(g, "Match Expression", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_EXTERNAL_MATCH),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_START),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_IMPLICATION_END),

        matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_BEGIN),        
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_MATCH_IMPLICATION),

        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_SEPARATOR),
            matte_syntax_graph_node_to_parent(3), // back to function arg begin split, hopefully i counted right
            NULL,

            matte_syntax_graph_node_token(MATTE_TOKEN_MATCH_END),
            matte_syntax_graph_node_end(),    
            NULL,
            NULL
        ),
        NULL

    );


    matte_syntax_graph_add_construct_path(g, "Literal Value", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_LITERAL_BOOLEAN,
            MATTE_TOKEN_LITERAL_NUMBER,
            MATTE_TOKEN_LITERAL_EMPTY,
            MATTE_TOKEN_LITERAL_STRING,

            MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION,
            MATTE_TOKEN_EXTERNAL_IMPORT,
            MATTE_TOKEN_EXTERNAL_IMPORTMODULE,
            MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN,
            MATTE_TOKEN_EXTERNAL_TYPENUMBER,
            MATTE_TOKEN_EXTERNAL_TYPEEMPTY,
            MATTE_TOKEN_EXTERNAL_TYPESTRING,
            MATTE_TOKEN_EXTERNAL_TYPEOBJECT,
            MATTE_TOKEN_EXTERNAL_TYPETYPE,
            MATTE_TOKEN_EXTERNAL_TYPEANY,
            MATTE_TOKEN_EXTERNAL_TYPENULLABLE,
            MATTE_TOKEN_EXTERNAL_TYPEFUNCTION,
            MATTE_TOKEN_EXTERNAL_PRINT,
            MATTE_TOKEN_EXTERNAL_SEND,
            MATTE_TOKEN_EXTERNAL_ERROR,
            MATTE_TOKEN_EXTERNAL_BREAKPOINT,
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

    matte_syntax_graph_add_construct_path(g, "Variable Access / Assignment", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
        matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
        matte_syntax_graph_node_split(

            matte_syntax_graph_node_token_group(
                MATTE_TOKEN_ASSIGNMENT_POW,
                MATTE_TOKEN_ASSIGNMENT_ADD,
                MATTE_TOKEN_ASSIGNMENT_SUB,
                MATTE_TOKEN_ASSIGNMENT_MULT,
                MATTE_TOKEN_ASSIGNMENT_DIV,
                MATTE_TOKEN_ASSIGNMENT_MOD,
                MATTE_TOKEN_ASSIGNMENT_AND,
                MATTE_TOKEN_ASSIGNMENT_OR,
                MATTE_TOKEN_ASSIGNMENT_XOR,
                MATTE_TOKEN_ASSIGNMENT_BLEFT,
                MATTE_TOKEN_ASSIGNMENT_BRIGHT,
                0
            ),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),    
            NULL,


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


    matte_syntax_graph_add_construct_path(g, "Simple Value", MATTE_SYNTAX_CONSTRUCT_EXPRESSION,
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
    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),                        
            matte_syntax_graph_node_end(),
            NULL,

            // vararg call 
            matte_syntax_graph_node_token(MATTE_TOKEN_VARARG),
            matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
            matte_syntax_graph_node_end(),
            NULL,            


            matte_syntax_graph_node_token(MATTE_TOKEN_VARIABLE_NAME),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),                        
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                matte_syntax_graph_node_to_parent(3),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_TYPESPEC),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),                
                matte_syntax_graph_node_split(
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),                        
                    matte_syntax_graph_node_end(),
                    NULL,                
                    
                    matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
                    matte_syntax_graph_node_to_parent(7),
                    NULL,

                    matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_SPECIFIER),
                    matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                    matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),                
                    matte_syntax_graph_node_to_parent(8),
                    NULL,
                    NULL
                ),
                NULL,                
                
                matte_syntax_graph_node_token(MATTE_TOKEN_GENERAL_SPECIFIER),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),                
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
    matte_syntax_graph_add_construct_path(g, "Object Literal Constructor", MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_BEGIN),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_SPREAD),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR),
                matte_syntax_graph_node_to_parent(6),
                NULL,
                NULL
            ),          
            NULL,
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_BINDING),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_END),
                matte_syntax_graph_node_end(),
                NULL,
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR),
                matte_syntax_graph_node_to_parent(6),
                NULL,
                NULL
            ),
            NULL,
            
            NULL
        ),
        NULL 
    );


    matte_syntax_graph_add_construct_path(g, "Function", MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION),
        matte_syntax_graph_node_end(),
        NULL
    );


    // array-like
    matte_syntax_graph_add_construct_path(g, "Array-like Literal", MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,
        matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_START),
        matte_syntax_graph_node_split(
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
            matte_syntax_graph_node_end(),
            NULL,
            matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_SPREAD),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR),
                matte_syntax_graph_node_to_parent(6),
                NULL,
                NULL
                
            ),        
            NULL,
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),

            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_END),
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_token(MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR),
                matte_syntax_graph_node_to_parent(5),
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
    /// Function definition {...} OR <- expression
    ///////////////
    ///////////////

    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION,
        matte_syntax_graph_node_split(                   
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_CONSTRUCTOR_INLINE),
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
            matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
            matte_syntax_graph_node_end(),
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
    /// New Object: function constructor
    ///////////////
    ///////////////

    // function
    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION,
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_CONSTRUCTOR),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_BODY),
        matte_syntax_graph_node_end(),        
        NULL
    );

    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION_WITH_SPECIFIER,
        matte_syntax_graph_node_token_alias(MATTE_TOKEN_FUNCTION_CONSTRUCTOR_WITH_SPECIFIER, MATTE_TOKEN_FUNCTION_CONSTRUCTOR),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_BODY),
        matte_syntax_graph_node_end(),
        NULL                 
    );

    
    ///////////////
    ///////////////
    /// Function body construct (after the ::)
    ///////////////
    ///////////////
    matte_syntax_graph_add_construct_path(g, "", MATTE_SYNTAX_CONSTRUCT_FUNCTION_BODY,
        matte_syntax_graph_node_split(                   
            matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH),
            matte_syntax_graph_node_to_parent(2),
            NULL,
        
            matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS),
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_TYPESPEC),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),      
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION),                
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION),                
                matte_syntax_graph_node_end(),

                NULL,
                NULL
            ),
            NULL,
            
            matte_syntax_graph_node_split(
                matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_TYPESPEC),
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
                matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),      
                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION),                
                matte_syntax_graph_node_end(),
                NULL,

                matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION),                
                matte_syntax_graph_node_end(),
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

    matte_syntax_graph_add_construct_path(g, "Return Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_RETURN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    
    matte_syntax_graph_add_construct_path(g, "When Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
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
    

    matte_syntax_graph_add_construct_path(g, "Foreach Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_FOREACH),

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


    matte_syntax_graph_add_construct_path(g, "Forever Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_FOREVER),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    matte_syntax_graph_add_construct_path(g, "For Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,

        matte_syntax_graph_node_token(MATTE_TOKEN_FOR),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_BEGIN),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_SEPARATOR),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_FUNCTION_ARG_END),
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    matte_syntax_graph_add_construct_path(g, "Declaration + Assignment", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token_group(
            MATTE_TOKEN_DECLARE_CONST,
            MATTE_TOKEN_DECLARE,
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

    
    matte_syntax_graph_add_construct_path(g, "Expression", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_construct(MATTE_SYNTAX_CONSTRUCT_EXPRESSION),
        matte_syntax_graph_node_marker(MATTE_TOKEN_MARKER_EXPRESSION_END),
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );

    
    matte_syntax_graph_add_construct_path(g, "Empty Statement", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_token(MATTE_TOKEN_STATEMENT_END),
        matte_syntax_graph_node_end(),
        NULL 
    );
    /*
    matte_syntax_graph_add_construct_path(g, "Nothing", MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT,
        matte_syntax_graph_node_end(),
        NULL 
    );
    */

}

matteSyntaxGraph_t * matte_syntax_graph_create() {
    matteSyntaxGraph_t * out = (matteSyntaxGraph_t*)matte_allocate(sizeof(matteSyntaxGraph_t));
    out->tokenNames = matte_array_create(sizeof(matteString_t *));
    out->constructRoots = matte_array_create(sizeof(matteSyntaxGraphRoot_t *));
    generate_graph(out);
    return out;
}


static void matte_syntax_graph_node_destroy(matteSyntaxGraphNode_t * node) {
    uint32_t i, len;
    switch(node->type) {
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN:
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS:
        matte_deallocate(node->token.refs);
        break;
      case MATTE_SYNTAX_GRAPH_NODE__SPLIT:
        len = node->split.count;
        for(i = 0; i < len; ++i) {
            matte_syntax_graph_node_destroy(node->split.nodes[i]);
        }
        matte_deallocate(node->split.nodes);
        
      
      default:;
    }
    
    // dumb but convenient, and node lengths are pretty small (N < 30)
    if (node->next)
        matte_syntax_graph_node_destroy(node->next);
    matte_deallocate(node);
}

static void matte_syntax_graph_root_destroy(matteSyntaxGraphRoot_t * root) {
    matte_string_destroy(root->name);
    uint32_t i;
    uint32_t len = matte_array_get_size(root->pathNames);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(root->pathNames, matteString_t *, i));
    }
    matte_array_destroy(root->pathNames);
    
    matteSyntaxGraphNode_t * node;
    len = matte_array_get_size(root->paths);
    for(i = 0; i < len; ++i) {
        node = matte_array_at(root->paths, matteSyntaxGraphNode_t *, i);
        matte_syntax_graph_node_destroy(node);
    }
    matte_array_destroy(root->paths);
    
    matte_deallocate(root);
    
}

void matte_syntax_graph_destroy(matteSyntaxGraph_t * graph) {
    uint32_t i;
    uint32_t len = matte_array_get_size(graph->tokenNames);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(graph->tokenNames, matteString_t *, i));
    }
    matte_array_destroy(graph->tokenNames);

    len = matte_array_get_size(graph->constructRoots);
    for(i = 0; i < len; ++i) {
        matte_syntax_graph_root_destroy(matte_array_at(graph->constructRoots, matteSyntaxGraphRoot_t*, i));
    }
    matte_array_destroy(graph->constructRoots);
    matte_deallocate(graph);
    
}



void matte_syntax_graph_add_construct_path(
    matteSyntaxGraph_t * g,
    const char * nameCStr,
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
                    prev->type == MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS ||
                    prev->type == MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT)
                    prev->next = n;
            } else {
                matte_array_push(root->paths, n);
                matteString_t * cp = matte_string_create_from_c_str("%s", nameCStr);
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
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN;
    n->token.count = 1;
    n->token.refs = (int*)matte_allocate(sizeof(int));
    n->token.refs[0] = a;
    n->token.marker = 0;
    return n;
}


matteSyntaxGraphNode_t * matte_syntax_graph_node_token_alias(
    int a,
    int b
){
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS;
    n->token.count = 1;
    n->token.refs = (int*)matte_allocate(sizeof(int)*2);
    n->token.refs[0] = a;
    n->token.refs[1] = b;
    n->token.marker = 0;
    return n;
}



// returns a new node that represents a single token
matteSyntaxGraphNode_t * matte_syntax_graph_node_marker(
    int a
){
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__TOKEN;
    n->token.count = 1;
    n->token.refs = (int*)matte_allocate(sizeof(int));
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
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
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
    n->token.refs = (int*)matte_allocate(sizeof(int)*n->token.count);
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
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__END;
    return n;
}


// returns a new node that represents a return to 
// a previous node. Level is how far up the tree this 
// previous node is. 0 is the self, 1 is the parent, etc
matteSyntaxGraphNode_t * matte_syntax_graph_node_to_parent(int level) {
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
    n->type = MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT;
    n->upLevel = level;
    return n;  
}


// returns a new node that represents a construct sub-tree.
matteSyntaxGraphNode_t * matte_syntax_graph_node_construct(int a){
    matteSyntaxGraphNode_t * n = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
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
    matteSyntaxGraphNode_t * out = (matteSyntaxGraphNode_t*)matte_allocate(sizeof(matteSyntaxGraphNode_t));
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
                    prev->type == MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS ||
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
    out->split.nodes = (matteSyntaxGraphNode_t**)matte_allocate(sizeof(matteSyntaxGraphNode_t *)*len);
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
        matteSyntaxGraphRoot_t * root = (matteSyntaxGraphRoot_t*)matte_allocate(sizeof(matteSyntaxGraphRoot_t));
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
    int constructID, const char * nameCStr,
    ...
) {
    matte_syntax_graph_set_construct_count(g, constructID);
    matte_string_clear(matte_syntax_graph_get_root(g, constructID)->name);
    matte_string_concat_printf(matte_syntax_graph_get_root(g, constructID)->name, "%s", nameCStr);

    va_list args;
    va_start(args, nameCStr);
    for(;;) {
        int id = va_arg(args, int);
        if (id == 0) break;

        const char * str = va_arg(args, char *);
        matte_syntax_graph_set_construct_count(g, id);
        matteString_t * strname = matte_syntax_graph_get_root(g, id)->name;
        matte_string_clear(strname);
        matte_string_concat_printf(strname, "%s", str);
    }
    va_end(args);
}




// For defining nodes of the syntax graph, the 
// nodes that accept 
void matte_syntax_graph_register_tokens(
    matteSyntaxGraph_t * g, 
    int token, const char * nameCStr,
    ...
){
    matte_syntax_graph_set_token_count(g, token);
    matte_string_clear(matte_array_at(g->tokenNames, matteString_t *, token));
    matte_string_concat_printf(matte_array_at(g->tokenNames, matteString_t *, token), "%s", nameCStr);

    va_list args;
    va_start(args, nameCStr);
    for(;;) {
        int id = va_arg(args, int);
        if (id == 0) break;

        const char * str = va_arg(args, char *);
        matte_syntax_graph_set_token_count(g, id);
        matteString_t * strname = matte_array_at(g->tokenNames, matteString_t *, id);
        matte_string_clear(strname);
        matte_string_concat_printf(strname, "%s", str);
    }
    va_end(args);
}



const matteString_t * matte_syntax_graph_get_token_name(matteSyntaxGraph_t * g, int en) {
    return matte_array_at(g->tokenNames, matteString_t *, en);
}

int matte_syntax_graph_is_construct(matteSyntaxGraph_t * g, int constructID) {
    return constructID >= 0 && constructID < matte_array_get_size(g->constructRoots);
}
