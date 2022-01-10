#ifndef H_MATTE__COMPILER__SYNTAX_GRAPH__INCLUDED
#define H_MATTE__COMPILER__SYNTAX_GRAPH__INCLUDED

typedef struct matteSyntaxGraph_t matteSyntaxGraph_t;
typedef struct matteString_t matteString_t;
typedef struct matteArray_t matteArray_t;

#include <stdint.h>

enum {
    MATTE_SYNTAX_GRAPH_NODE__TOKEN,
    MATTE_SYNTAX_GRAPH_NODE__END,
    MATTE_SYNTAX_GRAPH_NODE__SPLIT,
    MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT,
    MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT
};



typedef enum {
    MATTE_TOKEN_EMPTY,

    // token is a number literal
    MATTE_TOKEN_LITERAL_NUMBER,
    MATTE_TOKEN_LITERAL_STRING,
    MATTE_TOKEN_LITERAL_BOOLEAN,
    MATTE_TOKEN_LITERAL_EMPTY,

    MATTE_TOKEN_EXTERNAL_NOOP,
    MATTE_TOKEN_EXTERNAL_GATE,
    MATTE_TOKEN_EXTERNAL_LOOP,
    MATTE_TOKEN_EXTERNAL_FOR,
    MATTE_TOKEN_EXTERNAL_FOREACH,
    MATTE_TOKEN_EXTERNAL_MATCH,
    MATTE_TOKEN_EXTERNAL_TYPE,
    MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION,
    MATTE_TOKEN_EXTERNAL_IMPORT,
    
    MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN,
    MATTE_TOKEN_EXTERNAL_TYPEEMPTY,
    MATTE_TOKEN_EXTERNAL_TYPENUMBER,
    MATTE_TOKEN_EXTERNAL_TYPESTRING,
    MATTE_TOKEN_EXTERNAL_TYPEOBJECT,
    MATTE_TOKEN_EXTERNAL_TYPETYPE,
    MATTE_TOKEN_EXTERNAL_TYPEFUNCTION,

    
    MATTE_TOKEN_EXTERNAL_PRINT,
    MATTE_TOKEN_EXTERNAL_LISTEN,
    MATTE_TOKEN_EXTERNAL_ERROR,
    MATTE_TOKEN_EXTERNAL_SEND,


    MATTE_TOKEN_EXPRESSION_GROUP_BEGIN, // (
    MATTE_TOKEN_EXPRESSION_GROUP_END, // )

    MATTE_TOKEN_ASSIGNMENT,
    MATTE_TOKEN_ASSIGNMENT_ADD,
    MATTE_TOKEN_ASSIGNMENT_SUB,
    MATTE_TOKEN_ASSIGNMENT_MULT,
    MATTE_TOKEN_ASSIGNMENT_DIV,
    MATTE_TOKEN_ASSIGNMENT_MOD,
    MATTE_TOKEN_ASSIGNMENT_POW,
    MATTE_TOKEN_ASSIGNMENT_AND,
    MATTE_TOKEN_ASSIGNMENT_OR,
    MATTE_TOKEN_ASSIGNMENT_XOR,
    MATTE_TOKEN_ASSIGNMENT_BLEFT,
    MATTE_TOKEN_ASSIGNMENT_BRIGHT,

    MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START,
    MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END,
    MATTE_TOKEN_OBJECT_ACCESSOR_DOT,
    MATTE_TOKEN_GENERAL_OPERATOR1,
    MATTE_TOKEN_GENERAL_OPERATOR2,
    MATTE_TOKEN_VARIABLE_NAME,
    MATTE_TOKEN_OBJECT_LITERAL_BEGIN, //{
    MATTE_TOKEN_OBJECT_LITERAL_END, //{
    MATTE_TOKEN_OBJECT_DEF_PROP, //:
    MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR, //,
    MATTE_TOKEN_OBJECT_ARRAY_START,
    MATTE_TOKEN_OBJECT_ARRAY_END,
    MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR,

    MATTE_TOKEN_DECLARE,
    MATTE_TOKEN_DECLARE_CONST,

    MATTE_TOKEN_FUNCTION_BEGIN, //  
    MATTE_TOKEN_FUNCTION_TYPESPEC, // =>
    MATTE_TOKEN_FUNCTION_END,   
    MATTE_TOKEN_FUNCTION_ARG_BEGIN,   
    MATTE_TOKEN_FUNCTION_ARG_SEPARATOR,   
    MATTE_TOKEN_FUNCTION_ARG_END,   
    MATTE_TOKEN_FUNCTION_CONSTRUCTOR, // ::
    MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH, // <=
    MATTE_TOKEN_FUNCTION_PARAMETER_SPECIFIER, // :


    MATTE_TOKEN_WHEN,
    MATTE_TOKEN_GATE_RETURN,
    MATTE_TOKEN_IMPLICATION_START, //(
    MATTE_TOKEN_IMPLICATION_END, //)

    MATTE_TOKEN_MATCH_BEGIN, //{
    MATTE_TOKEN_MATCH_END, //}
    MATTE_TOKEN_MATCH_IMPLIES, //:
    MATTE_TOKEN_MATCH_SEPARATOR, //,
    MATTE_TOKEN_MATCH_DEFAULT, //default
    MATTE_TOKEN_RETURN,

    MATTE_TOKEN_STATEMENT_END, // newline OR ;
    
    MATTE_TOKEN_MARKER_EXPRESSION_END,

} matteTokenType_t;

enum {
    MATTE_SYNTAX_CONSTRUCT_EXPRESSION = 1, 
    MATTE_SYNTAX_CONSTRUCT_NEW_FUNCTION,
    MATTE_SYNTAX_CONSTRUCT_NEW_OBJECT,  
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_CALL, 
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT, 
    MATTE_SYNTAX_CONSTRUCT_VALUE, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CREATION_ARGS, 
    MATTE_SYNTAX_CONSTRUCT_VALUE_FUNCTION_CALL_ARGS,
    MATTE_SYNTAX_CONSTRUCT_FUNCTION_DEFINITION,
    MATTE_SYNTAX_CONSTRUCT_POSTFIX
};


// since compilation only allow for external functions 
// on error which result in termination of valid compilation,
// settings are controlled using statics.
static int OPTION__NAMED_REFERENCES = 0;

typedef struct matteSyntaxGraphWalker_t matteSyntaxGraphWalker_t;
typedef struct matteSyntaxGraphNode_t matteSyntaxGraphNode_t;


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



matteSyntaxGraph_t * matte_syntax_graph_create();

const matteString_t * matte_syntax_graph_get_token_name(matteSyntaxGraph_t *, int);

matteSyntaxGraphRoot_t * matte_syntax_graph_get_root(
    matteSyntaxGraph_t *,
    uint32_t id
);
int matte_syntax_graph_is_construct(matteSyntaxGraph_t * g, int id);
#endif
