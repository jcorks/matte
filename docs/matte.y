/*
    Using YACC form, this file describes the CFG (context-free grammar)
    description of the Matte language. 
    
    This description is not a complete lex-yecc program, but could 
    hypothetically serve as the yacc portion of said program.
    
    Each token has commented above descriptions about how 
    the lexical portion of the token should be 
    computed. 


    Certain syntatic and lexical concepts are borrowed from the C language's 
    specification. When so, it will be clearly noted.


    Characters considered to be part of tokens meet the following conditions:
        - The character is not in a comment. Comments are described by 
          C's single line and multiline comment syntax.
        - The character is not of the following values:
            \t
            \r
            space
            \v
            \f
            \n

*/

/*     
    Number token that can successfully be read by %lf in C's scanf.
    Else, a token that can be successfully read by %x in C's scanf.
*/
%token LITERAL_NUMBER                               

/*     
    C-string literal with the following differences:
        - The character for the string itself can be a matching pair of " OR '.
          each can be escaped.
        - Newline characters are accepted as part of the string, making the string 
          able to support multiple lines naturally.
        - Only the following escaped characters are supported:
            \"
            \'
            \\
            \n
            \t
            \b
            \r
*/ 
%token LITERAL_STRING   

/*     Exactly: true
       Exactly: false 
*/
%token LITERAL_BOOLEAN 

/*     Exactly: empty */
%token LITERAL_EMPTY



/*     Exactly: if */
%token GATE

/*     Exactly: match */
%token MATCH

/*     Exactly: getExternalFunction */
%token GETEXTERNALFUNCTION

/*     Exactly: import */
%token IMPORT



/*     Exactly: Boolean */
%token TYPEBOOLEAN

/*     Exactly: Empty */
%token TYPEEMPTY

/*     Exactly: Number */
%token TYPENUMBER

/*     Exactly: String */
%token TYPESTRING

/*     Exactly: Object */
%token TYPEOBJECT

/*     Exactly: Type */
%token TYPETYPE

/*     Exactly: Function */
%token TYPEFUNCTION



/*     Exactly: print */
%token PRINT

/*     Exactly: error */
%token ERROR

/*     Exactly: send */
%token SEND

/*     Exactly: breakpoint */
%token BREAKPOINT



/*     Exactly: ( */
%token EXPRESSION_GROUP_BEGIN

/*     Exactly: ) */
%token EXPRESSION_GROUP_END

/*     Exactly:  = */
%token ASSIGNMENT

/*     Exactly:  += */
%token ASSIGNMENT_ADD

/*     Exactly:  -= */
%token ASSIGNMENT_SUB

/*     Exactly:  *= */
%token ASSIGNMENT_MULT

/*     Exactly:  /= */
%token ASSIGNMENT_DIV

/*     Exactly:  %= */
%token ASSIGNMENT_MOD

/*     Exactly:  **= */
%token ASSIGNMENT_POW

/*     Exactly:  &= */
%token ASSIGNMENT_AND

/*     Exactly:  |= */
%token ASSIGNMENT_OR

/*     Exactly:  ^= */
%token ASSIGNMENT_XOR

/*     Exactly:  <<= */
%token ASSIGNMENT_BLEFT

/*     Exactly:  >>= */
%token ASSIGNMENT_BRIGHT



/*     Exactly: [ */
%token OBJECT_ACCESSOR_BRACKET_START

/*     Exactly: ] */
%token OBJECT_ACCESSOR_BRACKET_END

/*     Exactly: . */
%token OBJECT_ACCESSOR_DOT

/*
    Exactly: -    
    Exactly: ~    
    Exactly: !    
    Exactly: #    
*/
%token GENERAL_OPERATOR1


/*
    Exactly: +    
    Exactly: /    
    Exactly: ?    
    Exactly: %    
    Exactly: ^    
    Exactly: |   
    Exactly: ||
    Exactly: &   
    Exactly: &&
    Exactly: <   
    Exactly: <<
    Exactly: <=
    Exactly: <>
    Exactly: >
    Exactly: >>
    Exactly: >=
    Exactly: *
    Exactly: **
    Exactly: =>
    Exactly: ==
    Exactly: !=
    Exactly: -
*/
%token GENERAL_OPERATOR2

/* Exactly: -> */
%token QUERY_OPERATOR

/*
    - Does not start with [0-9]
    - Is a set of character codepoints in UTF8
    - Each character codepoint "c" fits this condition in code:
                (c > 47 && c < 58)  || // nums
                (c > 64 && c < 91)  || // uppercase
                (c > 96 && c < 123) || // lowercase
                (c == '_') || (c == '$') || // underscore or binding token
                (c > 127)              // other things / unicode
    
    
*/
%token IDENTIFIER


/* Exactly: { */
%token OBJECT_LITERAL_BEGIN

/* Exactly: } */
%token OBJECT_LITERAL_END

/* Exactly: , */
%token OBJECT_LITERAL_SEPARATOR

/* Exactly: [ */
%token OBJECT_ARRAY_START

/* Exactly: ] */
%token OBJECT_ARRAY_END

/* Exactly: , */
%token OBJECT_ARRAY_SEPARATOR

/* Exactly ... */
%token OBJECT_SPREAD





/* Exactly: @ */
%token DECLARE

/* Exactly: @: */
%token DECLARE_CONST

/* Exactly: { */
%token FUNCTION_BEGIN

/* Exactly: => */
%token FUNCTION_TYPESPEC

/* Exactly: } */
%token FUNCTION_END

/* Exactly: ( */
%token FUNCTION_ARG_BEGIN

/* Exactly: , */
%token FUNCTION_ARG_SEPARATOR

/* Exactly: ) */
%token FUNCTION_ARG_END

/* Exactly: :: */
%token FUNCTION_CONSTRUCTOR

/* Exactly: ::: */
%token FUNCTION_CONSTRUCTOR_WITH_SPECIFIER

/* Exactly: <- */
%token FUNCTION_CONSTRUCTOR_INLINE

/* Exactly: ::<= */
%token FUNCTION_CONSTRUCTOR_DASH

/* Exactly: {:::} */
%token FUNCTION_LISTEN



/* Exactly: when */
%token WHEN

/* Exactly: for */
%token FOR

/* Exactly: forever */
%token FOREVER

/* Exactly: foreach */
%token FOREACH



/* Exactly: : */
%token FOR_SEPARATOR

/* Exactly: else */
%token GATE_RETURN

/* Exactly: ( */
%token IMPLICATION_START

/* Exactly: ) */
%token IMPLICATION_END




/* Exactly: { */
%token MATCH_BEGIN

/* Exactly: } */
%token MATCH_END

/* Exactly: , */
%token MATCH_SEPARATOR

/* Exactly: default */
%token MATCH_DEFAULT

/* Exactly: : */
%token GENERAL_SPECIFIER

/* Exactly: return */
%token RETURN 

/* Exactly: [newline character] OR ; */
%token STATEMENT_END

/* Exactly: * */
%token VARARG

%%



/*
    Standalone value
*/
value : IDENTIFIER
      | new_object
      | new_function
      ;



/*
    General Value binding
*/  
value_binding : new_function_with_specifier
              | new_function
              | GENERAL_SPECIFIER expression
              ;
              
              
              
/*
    Function call

*/
              
function_call : FUNCTION_ARG_BEGIN function_call__content
              ;

function_call__content : FUNCTION_ARG_END
                       | VARARG expression FUNCTION_ARG_END
                       | IDENTIFIER function_call__content_arg
                       | value_binding FUNCTION_ARG_END
                       ;
                       
function_call__content_arg : FUNCTION_ARG_SEPARATOR function_call__content_arg
                           | value_binding function_call__content_arg_value
                           | FUNCTION_ARG_END
                           ;
                           
function_call__content_arg_value : FUNCTION_ARG_END
                                 | FUNCTION_ARG_SEPARATOR function_call__content_arg 
                                 ;                   
                            
                            
                            
                            
postfix : QUERY_OPERATOR expression 
        | GENERAL_OPERATOR2 expression 
        | OBJECT_ACCESSOR_DOT postfix__daa
        | OBJECT_ACCESSOR_BRACKET_START expression OBJECT_ACCESSOR_BRACKET_END postfix__a_post
        | OBJECT_ACCESSOR_BRACKET_START expression OBJECT_ACCESSOR_BRACKET_END
        | function_call
        ;

/*
    Dot accessor + assignment  
*/  
postfix__daa : ASSIGNMENT expression
             | IDENTIFIER postfix__a_post
             | IDENTIFIER
             ; 
                     
postfix__a_post   : ASSIGNMENT_POW expression
                  | ASSIGNMENT_ADD expression
                  | ASSIGNMENT_SUB expression
                  | ASSIGNMENT_MULT expression
                  | ASSIGNMENT_DIV expression
                  | ASSIGNMENT_MOD expression
                  | ASSIGNMENT_AND expression
                  | ASSIGNMENT_OR expression
                  | ASSIGNMENT_XOR expression
                  | ASSIGNMENT_BLEFT expression
                  | ASSIGNMENT_BRIGHT expression
                  | postfix
                  | postfix_repeat
                  | ASSIGNMENT expression
                  ;
                  

/* general expression */
expression : FUNCTION_LISTEN function_definition GENERAL_SPECIFIER expression
           | FUNCTION_LISTEN function_definition
           | GENERAL_OPERATOR1 expression postfix_repeat
           | GENERAL_OPERATOR1 expression
           | EXPRESSION_GROUP_BEGIN expression EXPRESSION_GROUP_END postfix_repeat
           | EXPRESSION_GROUP_BEGIN expression EXPRESSION_GROUP_END
           | GATE IMPLICATION_START expression IMPLICATION_END expression
           | GATE IMPLICATION_START expression IMPLICATION_END expression GATE_RETURN expression
           | MATCH IMPLICATION_START expression IMPLICATION_END MATCH_BEGIN match__content
           | simple_value postfix
           | simple_value
           | IDENTIFIER postfix__a_post
           | value postfix_repeat
           | value 
           ;
           
postfix_repeat : postfix 
               | postfix postfix_repeat
               ;


match__content : match_implication MATCH_END
               | MATCH_SEPARATOR match__content
               ;

simple_value : LITERAL_BOOLEAN 
                 | LITERAL_NUMBER
                 | LITERAL_EMPTY
                 | LITERAL_STRING
                 | GETEXTERNALFUNCTION
                 | IMPORT
                 | TYPEBOOLEAN
                 | TYPENUMBER
                 | TYPEEMPTY
                 | TYPESTRING
                 | TYPEOBJECT
                 | TYPETYPE
                 | TYPEFUNCTION
                 | PRINT
                 | SEND
                 | ERROR
                 | BREAKPOINT
                 ;



/* match */
match_implication : MATCH_DEFAULT value_binding
                  | IMPLICATION_START match_implication__chunk
                  ;
                  
match_implication__chunk : expression IMPLICATION_END value_binding
                         | FUNCTION_ARG_SEPARATOR match_implication__chunk
                         ;
                         
                         
                         
/* function creation */

value_function_creation_args : FUNCTION_ARG_BEGIN FUNCTION_ARG_END
                             | FUNCTION_ARG_BEGIN VARARG IDENTIFIER FUNCTION_ARG_END
                             | FUNCTION_ARG_BEGIN value_function_creation_args__name_chunk
                             ;

value_function_creation_args__name_chunk 
    : IDENTIFIER value_function_creation_args__tail_chunk
    ; 

value_function_creation_args__tail_chunk 
             : FUNCTION_ARG_END
             | FUNCTION_ARG_SEPARATOR value_function_creation_args__name_chunk 
             | FUNCTION_TYPESPEC expression value_function_creation_args__typespec_tail
             | GENERAL_SPECIFIER expression value_function_creation_args__name_chunk
             ;

value_function_creation_args__typespec_tail
             : FUNCTION_ARG_END
             | FUNCTION_ARG_SEPARATOR value_function_creation_args__name_chunk 
             | GENERAL_SPECIFIER expression value_function_creation_args__name_chunk
             ;



/* Object literal */

new_object : OBJECT_LITERAL_BEGIN new_object__definition
           | new_array
           ;
           
new_object__definition : OBJECT_LITERAL_END
                       | OBJECT_SPREAD expression OBJECT_LITERAL_END
                       | OBJECT_SPREAD expression OBJECT_LITERAL_SEPARATOR new_object__definition
                       | expression value_binding OBJECT_LITERAL_END
                       | expression value_binding OBJECT_LITERAL_SEPARATOR new_object__definition
                       ;

new_array : OBJECT_ARRAY_START new_array__definition    
          ;


new_array__definition : OBJECT_ARRAY_END
                      | OBJECT_SPREAD expression OBJECT_ARRAY_END
                      | OBJECT_SPREAD expression OBJECT_ARRAY_SEPARATOR new_array__definition
                      | expression OBJECT_ARRAY_END
                      | expression OBJECT_ARRAY_SEPARATOR new_array__definition
                      ;



function_definition : FUNCTION_CONSTRUCTOR_INLINE expression
                    | FUNCTION_BEGIN FUNCTION_END
                    | FUNCTION_BEGIN function_definition__repeat
                    ;

function_definition__repeat : function_scope_statement function_definition__repeat
                            | FUNCTION_END
                            ;






new_function : FUNCTION_CONSTRUCTOR function_body
             ;

new_function_with_specifier : FUNCTION_CONSTRUCTOR_WITH_SPECIFIER function_body
                            | FUNCTION_CONSTRUCTOR function_body
                            ;




function_body : FUNCTION_CONSTRUCTOR_DASH function_body
              | value_function_creation_args FUNCTION_TYPESPEC expression function_definition
              | value_function_creation_args function_definition
              | FUNCTION_TYPESPEC expression function_definition
              | function_definition
              ;
              
              
/* function scope statement */

function_scope_statement 
     : RETURN expression STATEMENT_END
     | WHEN FUNCTION_ARG_BEGIN expression FUNCTION_ARG_END expression STATEMENT_END
     | FOREACH FUNCTION_ARG_BEGIN expression FUNCTION_ARG_END expression STATEMENT_END
     | FOREVER expression STATEMENT_END
     | FOR FUNCTION_ARG_BEGIN expression FUNCTION_ARG_SEPARATOR expression FUNCTION_ARG_END expression STATEMENT_END
     | immutable_variable_declaration
     | mutable_variable_declaration
     | expression STATEMENT_END
     | STATEMENT_END
     ;

mutable_variable_declaration    
    : DECLARE IDENTIFIER new_function STATEMENT_END
    | DECLARE IDENTIFIER ASSIGNMENT expression STATEMENT_END
    | DECLARE IDENTIFIER STATEMENT_END
    ;

immutable_variable_declaration    
    : DECLARE_CONST IDENTIFIER new_function STATEMENT_END
    | DECLARE_CONST IDENTIFIER ASSIGNMENT expression STATEMENT_END
    | DECLARE_CONST IDENTIFIER STATEMENT_END
    ;




%%
