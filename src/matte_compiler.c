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
#include "matte_compiler.h"
#include "matte_array.h"
#include "matte_string.h"
#include "matte_table.h"
#include "matte_bytecode_stub.h"
#include "matte_opcode.h"
#include "matte_compiler__syntax_graph.h"
#include "matte.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>



#define FUNCTION_ARG_MAX 0xff
#define FUNCTION_LOCAL_MAX 0xff
#define FUNCTION_CAPTURES_MAX 0xffff


#define MATTE_TOKEN_END -1


// since compilation only allow for external functions 
// on error which result in termination of valid compilation,
// settings are controlled using statics.
static int OPTION__NAMED_REFERENCES = 0;

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
    
    // 0 -> text,
    // 1 -> array  of instructions 
    // 2 -> function stub.
    int dataType;
    void * data;

    // The next token if this token is part of a chain. Else, is NULL.
    matteToken_t * next;
};

enum {
    MATTE_TOKEN_DATA_TYPE__STRING,
    MATTE_TOKEN_DATA_TYPE__ARRAY_INST,
    MATTE_TOKEN_DATA_TYPE__FUNCTION_BLOCK
};

// replaces the existing data with new data and its type.
void matte_token_new_data(matteToken_t *, void * data, int dataType);

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
int matte_tokenizer_is_end(matteTokenizer_t *);

// prints a token to stdout
void matte_token_print(matteToken_t * t);















// Attempts to add additional tokens to the 
// token chain by scanning for the given constructs.
// returns success (1 if sucess, 0 if failure).
// On failure, the onError function is called with 
// reportable details.
static int matte_syntax_graph_continue(
    matteSyntaxGraphWalker_t * graph,
    int constructID
);




// Creates a new syntax tree,
//
// A syntax graph allows reduction of raw, UT8 text 
// into parsible tokens. 
static matteSyntaxGraphWalker_t * matte_syntax_graph_walker_create(
    matteSyntaxGraph_t * graphsrc,
    matteTokenizer_t *,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch, void * userdata), 
    void * userdata
);

static void matte_syntax_graph_walker_destroy(matteSyntaxGraphWalker_t *);

// Returns the first parsed token from the user source
//static matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraphWalker_t * g);


#define GET_LINE_OFFSET(__f__) (iter->line - (__f__)->startingLine)

// compiles all nodes into bytecode function blocks.
typedef struct matteFunctionBlock_t matteFunctionBlock_t;
struct matteFunctionBlock_t {
    uint32_t stubID;
    // count of typestrict types. args + 1
    uint32_t typestrict;

    // whether the function is run immediately before parsing of 
    // additional tokens.
    int isDashed;
    
    // If the function has a vararg parameter
    int isVarArg;
    
    // whether the function does nothign useful. This will be replaced with 
    // The Empty Function
    int isEmpty;
    
    // Starting line of the function block
    uint32_t startingLine;

    matteArray_t * typestrict_types;
    // array of matteString_t *
    matteArray_t * locals;
    // array of int, of size locals.
    matteArray_t * local_isConst;
    // array of matteString_t *
    matteArray_t * args;
    // array of static strings.
    matteArray_t * strings;
    // array of matteBytecodeStubInstruction_t
    matteArray_t * instructions;
    // pointer to parent calling function
    matteFunctionBlock_t * parent;

    // array of matteBytecodeStubCapture_t 
    matteArray_t * captures;
    // array of matteString_t *, names of the captures.
    matteArray_t * captureNames;
    // array of int, of size captureNames
    matteArray_t * capture_isConst;
    
};



// converts all graph nodes into an array of matteFunctionBlock_t
static matteArray_t * matte_syntax_graph_compile(matteSyntaxGraphWalker_t * g);
static matteString_t * matte_syntax_graph_print(matteSyntaxGraphWalker_t * g);

// transers ownership of the array and frees it and its contents.
static void * matte_function_block_array_to_bytecode(
    matteArray_t * arr, 
    uint32_t * size
);


matteString_t * matte_compiler_tokenize(
    matteSyntaxGraph_t * graphsrc,
    const uint8_t * source, 
    uint32_t len,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void * userdata),
    void * userdata
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraphWalker_t * st = matte_syntax_graph_walker_create(graphsrc, w, onError, userdata);


   
    // use the text walker to generate an array of tokens for 
    // the entire source. Basic case here allows you to 
    // filter out any poorly formed contextual tokens
    int success;
    while((success = matte_syntax_graph_continue(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT))) {
        if (matte_tokenizer_is_end(w)) break;
    }

    matte_tokenizer_destroy(w);
    if (!success) {
        matte_syntax_graph_walker_destroy(st);
        return NULL;
    }
    matteString_t * out = matte_syntax_graph_print(st);
    matte_syntax_graph_walker_destroy(st);
    return out;
}


static uint8_t * matte_compiler_run_base(
    matteSyntaxGraph_t * graphsrc,
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraphWalker_t * st = matte_syntax_graph_walker_create(graphsrc, w, onError, userdata);


   
    // use the text walker to generate an array of tokens for 
    // the entire source. Basic case here allows you to 
    // filter out any poorly formed contextual tokens
    int success;
    while((success = matte_syntax_graph_continue(st, MATTE_SYNTAX_CONSTRUCT_FUNCTION_SCOPE_STATEMENT))) {
        if (matte_tokenizer_is_end(w)) break;
    }
    matte_tokenizer_destroy(w);
    if (!success) {
        *size = 0;
        matte_syntax_graph_walker_destroy(st);
        return NULL;
    }

    // then walk through the tokens and group them together.
    // a majority of compilation errors will likely happen here, 
    // as only a strict series of tokens are allowed in certain 
    // groups. Might want to form function groups with parenting to track referrables.
    // optimization may happen here if desired.

    // finally emit code for groups.
    matteArray_t * arr = matte_syntax_graph_compile(st);
    if (!arr) {
        *size = 0;
        matte_syntax_graph_walker_destroy(st);
        return NULL;
    }

    void * bytecode = matte_function_block_array_to_bytecode(arr, size);
    matte_syntax_graph_walker_destroy(st);


    // cleanup :(
    // especially those gosh darn function blocks
    return (uint8_t*)bytecode;
}

uint8_t * matte_compiler_run_with_named_references(
    matteSyntaxGraph_t * graph,
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    OPTION__NAMED_REFERENCES = 1;
    return matte_compiler_run_base(
        graph, source, len, size, onError, userdata
    );
}

uint8_t * matte_compiler_run(
    matteSyntaxGraph_t * graph,
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    OPTION__NAMED_REFERENCES = 0;
    return matte_compiler_run_base(
        graph, source, len, size, onError, userdata
    );
}




/////////////////////////////////////////////////////////////

// walker;

static int token_is_assignment_derived(matteTokenType_t t) {
    switch(t) {
      case MATTE_TOKEN_ASSIGNMENT:
      case MATTE_TOKEN_ASSIGNMENT_ADD:
      case MATTE_TOKEN_ASSIGNMENT_SUB:
      case MATTE_TOKEN_ASSIGNMENT_MULT:
      case MATTE_TOKEN_ASSIGNMENT_DIV:
      case MATTE_TOKEN_ASSIGNMENT_MOD:
      case MATTE_TOKEN_ASSIGNMENT_POW:
      case MATTE_TOKEN_ASSIGNMENT_AND:
      case MATTE_TOKEN_ASSIGNMENT_OR:
      case MATTE_TOKEN_ASSIGNMENT_XOR:
      case MATTE_TOKEN_ASSIGNMENT_BLEFT:
      case MATTE_TOKEN_ASSIGNMENT_BRIGHT:
        return 1;
      default:;
    }
    return 0;
}   


static int utf8_next_char(uint8_t ** source) {
    uint8_t * iter = *source;
    int val = (*source)[0];
    if (val < 128 && *iter) {
        val = (iter[0]) & 0x7F;
        (*source)++;
        return val;
    } else if (val < 224 && *iter && iter[1]) {
        val = ((iter[0] & 0x1F)<<6) + (iter[1] & 0x3F);
        (*source)+=2;
        return val;
    } else if (val < 240 && *iter && iter[1] && iter[2]) {
        val = ((iter[0] & 0x0F)<<12) + ((iter[1] & 0x3F)<<6) + (iter[2] & 0x3F);
        (*source)+=3;
        return val;
    } else if (*iter && iter[1] && iter[2] && iter[3]) {
        val = ((iter[0] & 0x7)<<18) + ((iter[1] & 0x3F)<<12) + ((iter[2] & 0x3F)<<6) + (iter[3] & 0x3F);
        (*source)+=4;
        return val;
    }
    return 0;
}


static matteString_t * consume_variable_name(uint8_t ** src) {
    int c = utf8_next_char(src);

    matteString_t * varname = matte_string_create();
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
        return varname;
    }
    

    int valid = 1;
    while(valid) {
        valid = (c > 47 && c < 58)  || // nums
                (c > 64 && c < 91)  || // uppercase
                (c > 96 && c < 123) || // lowercase
                (c == '_') || (c == '$') ||  // interface private token or binding token
                (c > 127); // other things / unicode stuff

        if (valid) {
            matte_string_append_char(varname, c);
        } else {
            *src -= 1;
            break;
        }
        c = utf8_next_char(src);
    }
    return varname;
}


struct matteTokenizer_t {
    uint8_t * backup;
    uint8_t * iter;
    uint8_t * source;
    uint32_t line;
    uint32_t character;

};

static matteToken_t * new_token(
    matteString_t * str,
    uint32_t line, 
    uint32_t character, 
    matteTokenType_t type
) {
    matteToken_t * t = (matteToken_t*)matte_allocate(sizeof(matteToken_t));
    t->line = line;
    t->character = character;
    t->ttype = type;
    t->data = str;
    t->dataType = 0;
    return t;
}

void matte_token_new_data(matteToken_t * t, void * data, int dataType) {
    switch(t->dataType) {
      case MATTE_TOKEN_DATA_TYPE__ARRAY_INST:
        matte_array_destroy((matteArray_t*)t->data);
        break;
      case MATTE_TOKEN_DATA_TYPE__STRING:
        matte_string_destroy((matteString_t*)t->data);      
        break;
    }
    t->data = data;
    t->dataType = dataType;
}


static void destroy_token(
    matteToken_t * t
) {
    matte_token_new_data(t, NULL, -1);
    matte_deallocate(t);
}



matteTokenizer_t * matte_tokenizer_create(const uint8_t * data, uint32_t byteCount) {
    matteTokenizer_t * t = (matteTokenizer_t*)matte_allocate(sizeof(matteTokenizer_t));
    t->line = 1;
    t->character = 1;
    t->source = (uint8_t*)matte_allocate(byteCount+sizeof(int32_t));
    uint32_t end = 0;
    memcpy(t->source, data, byteCount);
    memcpy(t->source+byteCount, &end, sizeof(int32_t));
    t->iter = t->source;
    t->backup = t->iter;
    return t;
}

void matte_tokenizer_destroy(matteTokenizer_t * t) {
    matte_deallocate(t->source);
    matte_deallocate(t);
}

uint32_t matte_tokenizer_current_line(const matteTokenizer_t * t) {
    return t->line;
}

uint32_t matte_tokenizer_current_character(const matteTokenizer_t * t) {
    return t->character;
}


static void tokenizer_strip(matteTokenizer_t * t, int skipNewline) {
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
                    if (c == '\n') {
                        t->line++;
                        t->character = 1;
                    } else {
                        t->character++;                    
                    }

                }
                t->backup = t->iter;
                break;
              default: // real char
                t->iter = t->backup;
                return;
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
            if (skipNewline) {
                t->backup = t->iter;
                t->line++;
                t->character = 1;
            } else {
                t->iter = t->backup; 
                return;   
            }
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
    tokenizer_strip(t, 0);
    uint8_t * iterC = t->iter;  
    return utf8_next_char(&iterC);
}



static matteToken_t * matte_tokenizer_consume_char(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
    uint32_t preLine,
    uint32_t preCh,
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
        t->line = preLine;
        t->character = preCh;
        return NULL;
    }

}

static matteToken_t * matte_tokenizer_consume_exact(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
    uint32_t preLine,
    uint32_t preCh,
    matteTokenType_t ty,
    const char * cha
) {
    const char * str = cha;
     
    while(*cha) {
        int c = utf8_next_char(&t->iter);   
        if (c == *cha) {
            cha++;
        } else {
            t->iter = t->backup;
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }
    }
    t->character += strlen(str);
    t->backup = t->iter;

    return new_token(
        matte_string_create_from_c_str("%s", str),
        line,
        ch,
        ty
    );            
}

static matteToken_t * matte_tokenizer_consume_word(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
    uint32_t preLine,
    uint32_t preCh,
    matteTokenType_t ty,
    const char * word
) {
    matteString_t * str = consume_variable_name(&t->iter);
    matteString_t * token = matte_string_create_from_c_str(word);
    if (matte_string_test_eq(str, token)) {
        t->character += matte_string_get_length(str);
        t->backup = t->iter;
        matte_string_destroy(token);
        return new_token(
            str,
            line,
            ch,
            ty
        );
    } else {
        t->line = preLine;
        t->character = preCh;
        t->iter = t->backup;        
        matte_string_destroy(str);
        matte_string_destroy(token);
        return NULL;
    }
}



// Given a matteTokenType_t, the tokenizer will attempt to parse the next 
// collection of characters as a token of the given type. If the current text 
// is a valid token of that type, a new matteToken_t * is created. If not, 
// NULL is returned. The new token is owned by the matteTokenizer_t * instance and 
// are no longer valid once the matteTokenizer_t * instance is destroyed. 
matteToken_t * matte_tokenizer_next(matteTokenizer_t * t, matteTokenType_t ty) {
    uint32_t preLine = t->line;
    uint32_t preCh = t->character;
    uint8_t * backup = t->backup;
    tokenizer_strip(t, ty != MATTE_TOKEN_STATEMENT_END);
    t->backup = backup;
    uint32_t currentLine = t->line;
    uint32_t currentCh = t->character;
    switch(ty) {
      case MATTE_TOKEN_LITERAL_NUMBER: {
        // convert into ascii
        int isDone = 0;
        int decimalCount = 0;
        matteString_t * out = matte_string_create();
        uint8_t * prev;
        while(!isDone) {
            prev = t->iter;
            int c = utf8_next_char(&t->iter);
            switch(c) {
              case '.':
                decimalCount ++;
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
                matte_string_append_char(out, c);
                break;


              case 'x':
              case 'e':
              case 'E':
              case 'a':
              case 'b':
              case 'c':
              case 'd':
              case 'f':
              case 'A':
              case 'B':
              case 'C':
              case 'D':
              case 'F':
              case 'X': {
                // these CANNOT be the leading char
                if (matte_string_get_length(out) > 0) {
                    matte_string_append_char(out, c);
                } else {
                    t->iter = prev;
                    isDone = 1;                
                }
                break;
                
              }
                

              default:
                t->iter = prev;
                isDone = 1;
                break;
            }
        }
        double f;
        uint32_t fhex;
        // poorly formed number: included 2 decimals.
        if (decimalCount > 1) {
            matte_string_destroy(out);
            t->iter = t->backup;
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }
        if (sscanf(matte_string_get_c_str(out), "%lf", &f) == 1) {
            t->character+=matte_string_get_length(out);
            t->backup = t->iter;

            return new_token(
                out, //xfer ownership
                currentLine,
                currentCh,
                ty
            );
        } else if (sscanf(matte_string_get_c_str(out), "%x", &fhex) == 1) {
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
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }
        break;
      }            
    




      case MATTE_TOKEN_LITERAL_STRING: {
        int c = utf8_next_char(&t->iter);
        int single = 0;
        switch(c) {
          case '\'':
            single = 1;
          case '"':
            t->character++;
            break;
          default:
            t->iter = t->backup;
            t->line = preLine;
            t->character = preCh;
            return NULL;            
        }
    
        
        matteString_t * text = matte_string_create();
        matte_string_append_char(text, c);
        for(;;) {
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '\'':
                if (!single) {
                    matte_string_append_char(text, c);                    
                    break;
                } else {
                    goto L_END_STRING;                
                }
              case '"':
                if (single) {
                    matte_string_append_char(text, c);                    
                    break;
                } else {
                    goto L_END_STRING;
                }
                
 
              case 0:
                t->iter = t->backup;
                matte_string_destroy(text);
                t->line = preLine;
                t->character = preCh;
                return NULL;            
                
              case '\\': // escape character 
                c = utf8_next_char(&t->iter);
                switch(c) {
                  case 'n':
                    matte_string_append_char(text, '\n');
                    break;
                  case 't':
                    matte_string_append_char(text, '\t');
                    break;
                  case 'b':
                    matte_string_append_char(text, '\b');
                    break;
                  case 'r':
                    matte_string_append_char(text, '\r');
                    break;
                  case '"':
                    matte_string_append_char(text, '"');
                    break;
                  case '\'':
                    matte_string_append_char(text, '\'');
                    break;
                  case '\\':
                    matte_string_append_char(text, '\\');
                    break;

                  default:
                    matte_string_append_char(text, '\\');
                    matte_string_append_char(text, c);
                    break;            
                  
                }    
                break;           

              default:
                matte_string_append_char(text, c);
                break;
            }
        }
      L_END_STRING:
        // end, cleanup;
        t->backup = t->iter;  
        matte_string_append_char(text, c);
        t->character+=matte_string_get_length(text);
        return new_token(
            text,
            currentLine,
            currentCh,
            ty
        );
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
                t->line = preLine;
                t->character = preCh;
                
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
                t->line = preLine;
                t->character = preCh;
                return NULL;
            }

          default:
            t->iter = t->backup;
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }
        break;
      }
      case MATTE_TOKEN_LITERAL_EMPTY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "empty");
        break;
      }

      case MATTE_TOKEN_EXTERNAL_NOOP: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "noop");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_GATE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "if");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_MATCH: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "match");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "getExternalFunction");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "import");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORTMODULE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "importModule");
        break;
      }



      case MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Boolean");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEEMPTY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Empty");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPENUMBER: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Number");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPESTRING: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "String");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEOBJECT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Object");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPETYPE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Type");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEFUNCTION: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Function");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEANY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Any");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPENULLABLE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "Nullable");
        break;
      }



      case MATTE_TOKEN_EXTERNAL_PRINT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "print");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_ERROR: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "error");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_SEND: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "send");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_BREAKPOINT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "breakpoint");
        break;
      }
      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '(');
        break;
      } // (
      case MATTE_TOKEN_EXPRESSION_GROUP_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ')');
        break;

      } // )
      case MATTE_TOKEN_ASSIGNMENT: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '=');
        break;
      }

      case MATTE_TOKEN_QUERY_OPERATOR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "->");
      }

      case MATTE_TOKEN_ASSIGNMENT_ADD: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "+=");
        break;
      }
      case MATTE_TOKEN_ASSIGNMENT_SUB: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "-=");
        break;
      }
      case MATTE_TOKEN_ASSIGNMENT_MULT:  {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "*=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_DIV: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "/=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_MOD: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "%=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_POW: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "**=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_AND:  {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "&=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_OR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "|=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_XOR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "^=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_BLEFT: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "<<=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_BRIGHT: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, ">>=");
        break;
      }


      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '[');

        break;

      }

      case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ']');

        break;          
      }
      case MATTE_TOKEN_OBJECT_SPREAD: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "...");

        break;          
      }

      case MATTE_TOKEN_OBJECT_ACCESSOR_DOT: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '.');

        break;
      }
      case MATTE_TOKEN_GENERAL_OPERATOR1: {
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case '-':
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
            t->line = preLine;
            t->character = preCh;
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
            t->character++;
            t->backup = t->iter;
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
                t->iter = t->backup;
                return new_token(
                    matte_string_create_from_c_str("|", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '&':
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '&':
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("&%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return new_token(
                    matte_string_create_from_c_str("&", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '<':
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '<':
              case '=':
              case '>':
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("<%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return new_token(
                    matte_string_create_from_c_str("<", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;


          case '>':
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '>':
              case '=':
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str(">%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
                return new_token(
                    matte_string_create_from_c_str(">", c),
                    currentLine,
                    currentCh,
                    ty
                );            
            }
            break;

          case '*':
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '*':
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("*%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

              default:
                t->iter = t->backup;
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
              case '>':
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
                t->line = preLine;
                t->character = preCh;
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
                t->line = preLine;
                t->character = preCh;
                return NULL;              
            }
            break;
          default:
            t->iter = t->backup;
            t->line = preLine;
            t->character = preCh;
            return NULL;


          case '-':
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {

              default:
                t->iter = t->backup;
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
        matteString_t * varname = consume_variable_name(&t->iter);
        if (matte_string_get_length(varname)) {
            t->character += matte_string_get_length(varname);
            t->backup = t->iter;
            return new_token(
                varname,
                currentLine,
                currentCh,
                ty
            );
        } else {
            t->iter = t->backup;
            matte_string_destroy(varname);
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }

        break;
      }
      case MATTE_TOKEN_OBJECT_LITERAL_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '{');
        break;          
      }
      case MATTE_TOKEN_OBJECT_LITERAL_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '}');
        break;
      }
      case MATTE_TOKEN_VARARG: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '*');
        break;
      }
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_WITH_SPECIFIER: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, ":::");
        break;  
      }

      case MATTE_TOKEN_FUNCTION_LISTEN: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "{:::}");
        break;
      }
      
      case MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ',');
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '[');
        break;
      }
      case MATTE_TOKEN_OBJECT_ARRAY_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ']');
        break;          
      }
      case MATTE_TOKEN_OBJECT_ARRAY_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ',');       
      }

      case MATTE_TOKEN_DECLARE: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '@');
        break;
      }
      case MATTE_TOKEN_DECLARE_CONST: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "@:");
        break;          
      }

      case MATTE_TOKEN_FUNCTION_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '{');
        break;
      }
      case MATTE_TOKEN_FUNCTION_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '}');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '(');
        break;
      }
      case MATTE_TOKEN_IMPLICATION_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '(');
        break;
      }
      case MATTE_TOKEN_IMPLICATION_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ')');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ',');
        break;
      }
      case MATTE_TOKEN_FUNCTION_ARG_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ')');
        break;          
      }

      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "::");
        break;  
      }
      case MATTE_TOKEN_GENERAL_SPECIFIER: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ':');
        break;          
      }
      
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "<=");
        break;  
      }
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_INLINE: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "<-");
        break;  
      }
      case MATTE_TOKEN_FUNCTION_TYPESPEC: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "=>");
        break;  
      }

      case MATTE_TOKEN_WHEN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "when");
        break;            
      }
      case MATTE_TOKEN_FOR: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "for");
        break;            
      }
      case MATTE_TOKEN_FOREVER: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "forever");
        break;            
      }
      case MATTE_TOKEN_FOREACH: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, "foreach");
        break;            
      }
      case MATTE_TOKEN_FOR_SEPARATOR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, preLine, preCh, ty, ":");
        break;  
      }

      case MATTE_TOKEN_GATE_RETURN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "else");
        break; 
      }
      case MATTE_TOKEN_MATCH_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '{');
        break; 
      }
      case MATTE_TOKEN_MATCH_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, '}');
        break; 
      }
      case MATTE_TOKEN_MATCH_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, preLine, preCh, ty, ',');
        break; 
      }
      case MATTE_TOKEN_MATCH_DEFAULT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "default");
        break; 
      }
      case MATTE_TOKEN_RETURN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, preLine, preCh, ty, "return");  
        break;
      }

      case MATTE_TOKEN_STATEMENT_END: {// newline OR ;
        int c = utf8_next_char(&t->iter);
        switch(c) {
          case 0:
            t->character = 1;
            t->backup = t->iter;
            return new_token(
                matte_string_create_from_c_str(""),
                currentLine,
                currentCh,
                ty
            );

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
            t->line = preLine;
            t->character = preCh;
            return NULL;
        }
        break;
      }

      default:
        assert("!NOT HANDLED YET");
    }
    assert(!"Should not be reached");
}

// Returns whether the tokenizer has reached the end of the inout text.
int matte_tokenizer_is_end(matteTokenizer_t * t){
    tokenizer_strip(t, 0);
    return t->iter[0] == 0;
}


















//////////////////////////////////////////
// syntax graph 



struct matteSyntaxGraphWalker_t {
    // source syntax graph
    matteSyntaxGraph_t * graphsrc;

    // first parsed token
    matteToken_t * first;
    // currently last parsed token
    matteToken_t * last;
    // source tokenizer instance
    matteTokenizer_t * tokenizer;
    // which nodes have been attempted so far. This prevents cycles.
    matteTable_t * tried;



    // starts at 0 (for the root)
    uint32_t functionStubID;


    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch, void * userdata);
    void * onErrorData;
};


matteSyntaxGraphWalker_t * matte_syntax_graph_walker_create(
    matteSyntaxGraph_t * graphsrc,
    matteTokenizer_t * t,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch, void * userdata),
    void * userdata
) {
    matteSyntaxGraphWalker_t * out = (matteSyntaxGraphWalker_t *)matte_allocate(sizeof(matteSyntaxGraphWalker_t));
    out->graphsrc = graphsrc;
    out->tokenizer = t;
    out->first = out->last = NULL;
    out->onError = onError;
    out->onErrorData = userdata;
    out->tried = matte_table_create_hash_pointer();


    return out;
}


void matte_syntax_graph_walker_destroy(matteSyntaxGraphWalker_t * t) {
    matteToken_t * iter = t->first;
    while(iter) {
        matteToken_t * next = iter->next;
        destroy_token(iter);
        iter = next;
    }
    matte_table_destroy(t->tried);
    matte_deallocate(t);
}


static matteString_t * matte_syntax_graph_node_get_string(
    matteSyntaxGraph_t * graph,
    matteSyntaxGraphNode_t * node
) {
    matteString_t * message = matte_string_create();
    switch(node->type) {
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        uint32_t i;
        uint32_t len = node->token.count;
        if (len == 1) {
            matte_string_concat(message, matte_syntax_graph_get_token_name(graph, node->token.refs[0]));
        } else {
            for(i = 0; i < len; ++i) {
                if (i == len - 1) {
                    matte_string_concat_printf(message, ", ");
                } else if (i == 0) {

                } else {
                    matte_string_concat_printf(message, ", or ");
                }             
                matte_string_concat(message, matte_syntax_graph_get_token_name(graph, node->token.refs[i]));
            }
        }
        break;
      }
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS: {
        matte_string_concat(message, matte_syntax_graph_get_token_name(graph, node->token.refs[1]));
        break;
      }
      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(graph, node->construct);
        matte_string_concat(message, root->name);
        break;
      }

      case MATTE_SYNTAX_GRAPH_NODE__SPLIT: {
        uint32_t lenn = node->split.count;
        uint32_t n;
        for(n = 0; n < lenn; ++n) {
            //uint32_t i;
            matteSyntaxGraphNode_t * nodeSub = node->split.nodes[n];
            //uint32_t len = nodeSub->split.count;

            matte_string_concat(message, matte_syntax_graph_node_get_string(graph, nodeSub));
            if (n == lenn-2)
                matte_string_concat_printf(message, "; or ");
            else if (n != lenn-1)
                matte_string_concat_printf(message, "; ");            

        }
        break;

      }
    }
    
    return message;
}

static void matte_syntax_graph_print_error(
    matteSyntaxGraphWalker_t * graph,
    matteSyntaxGraphNode_t * node,
    int startedLine
) {
    matteString_t * message = matte_string_create_from_c_str("Syntax Error: Expected ");
    matteString_t * c = matte_syntax_graph_node_get_string(graph->graphsrc, node);
    matte_string_concat(message, c);
    matte_string_destroy(c);

    if (matte_tokenizer_peek_next(graph->tokenizer)) {
        matte_string_concat_printf(message, " but received '");
        matte_string_append_char(message, matte_tokenizer_peek_next(graph->tokenizer));
        matte_string_concat_printf(message, "' instead.");
    } else {
        matte_string_concat_printf(message, " but reached the end of the source instead.");
    }
    if (startedLine > 0 && startedLine != matte_tokenizer_current_line(graph->tokenizer)) {
        matte_string_concat_printf(message, " (Note: This syntactic construct started at line %d)", startedLine);    
    }

    if (graph->onError)
        graph->onError(
            message,
            matte_tokenizer_current_line(graph->tokenizer),
            matte_tokenizer_current_character(graph->tokenizer),
            graph->onErrorData
        );
    matte_string_destroy(message);
}

/*
static void print_graph_node(matteSyntaxGraphWalker_t * g, matteSyntaxGraphNode_t * n) {
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
            printf("    accepted: %s\n", matte_string_get_c_str(matte_syntax_graph_get_token_name(GRAPHSRC, n->token.refs[i])));
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
        printf("Construct node. (%s)\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, n->construct)->name));
        break;
      }

    }
}

*/



static matteSyntaxGraphNode_t * matte_syntax_graph_walk_helper(
    matteSyntaxGraphWalker_t * graph,
    matteSyntaxGraphNode_t * node,
    int constructID,
    int silent,
    int * error
) {
    if (matte_table_find(graph->tried, node)) return NULL;
    matte_table_insert(graph->tried, node, (void*)0x1);
    #ifdef MATTE_DEBUG__COMPILER
        static int level = 0;
        level++;
    #endif

    // If we expect a token but theres no characters left, drop out.    
    if (!(
        node->type == MATTE_SYNTAX_GRAPH_NODE__END ||
        node->type == MATTE_SYNTAX_GRAPH_NODE__SPLIT ||
        node->type == MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT
    )) {
        if (matte_tokenizer_is_end(graph->tokenizer))
            return NULL;
    }
    
    switch(node->type) {
      // node is simply a token / token set.
      // We try each token in order. If it parses successfully,
      // we continue. If not, we try the next token.
      // If we exhaust all tokens, we have failed.
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS:
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__TOKEN: %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(graph->graphsrc, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);
        #endif
        uint32_t i;
        uint32_t len = node->token.count;
        for(i = 0; i < len; ++i) {
            #ifdef MATTE_DEBUG__COMPILER
                printf("     - trying to parse token as %s...", matte_string_get_c_str(matte_syntax_graph_get_token_name(graph->graphsrc, node->token.refs[i])));
            #endif


            matteToken_t * newT;
            // markers do not consume text.
            if (node->token.marker) {
                newT = new_token(
                    matte_string_create_from_c_str(""),
                    graph->tokenizer->line,
                    graph->tokenizer->character,
                    (matteTokenType_t)node->token.refs[i]
                ); 

            // normal case: consume text needed based on token
            } else {
                newT = matte_tokenizer_next(graph->tokenizer, (matteTokenType_t)node->token.refs[i]);
                
                // set type to aliased type.
                if (newT && node->type == MATTE_SYNTAX_GRAPH_NODE__TOKEN_ALIAS)
                    newT->ttype = (matteTokenType_t)node->token.refs[1];

            }


            #ifdef MATTE_DEBUG__COMPILER
                printf("%s (line %d:%d)\n", newT ? "SUCCESS!": "failure", graph->tokenizer->line, graph->tokenizer->character);
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
                #ifdef MATTE_DEBUG__COMPILER
                    level--;
                #endif
                return node->next;
            }
        }
        // failure
        if (!silent) {
            matte_syntax_graph_print_error(graph, node, -1);
            *error = 1;
        }
        #ifdef MATTE_DEBUG__COMPILER
            level--;
        #endif
        return NULL;
      }


      // the node is generically able to be split into 
      // possible paths. Each are attempted in order.
      case MATTE_SYNTAX_GRAPH_NODE__SPLIT: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__SPLIT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(graph->graphsrc, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif

        uint32_t i;
        uint32_t len = node->split.count;
        for(i = 0; i < len; ++i) {
            matteSyntaxGraphNode_t * n = matte_syntax_graph_walk_helper(
                graph,
                node->split.nodes[i],
                constructID,
                1,
                error
            );
            if (*error) return NULL;

            // success!
            if (n) {
                matte_table_clear(graph->tried);
                #ifdef MATTE_DEBUG__COMPILER
                    level--;
                #endif
                return n;
            }
        }
        // failure
        if (!silent) {
            matte_syntax_graph_print_error(graph, node, -1);
            *error = 1;
        }
        #ifdef MATTE_DEBUG__COMPILER
            level--;
        #endif
        return NULL;
        break;
      }


      // the node gener
      case MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: @MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(graph->graphsrc, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif

        uint32_t level = node->upLevel;
        matteSyntaxGraphNode_t * n = node;
        while(level--) {
            if (n->parent) n = n->parent;
        }
        if (!n) {
            if (!silent) {
                matte_syntax_graph_print_error(graph, node, -1);
                *error = 1;
            }
            #ifdef MATTE_DEBUG__COMPILER
                level--;
            #endif
            return NULL;
        } else {
            matte_table_clear(graph->tried);
            #ifdef MATTE_DEBUG__COMPILER
                level--;
            #endif
            return n;
        }
      }

      // the end of a path has been reached. return
      case MATTE_SYNTAX_GRAPH_NODE__END: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__END %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(graph->graphsrc, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);
            matte_table_clear(graph->tried);
        #endif
        #ifdef MATTE_DEBUG__COMPILER
            level--;
        #endif
        return node;
      }


      // the node is a construct path
      // All top paths are tried before continuing
      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(graph->graphsrc, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(graph->graphsrc, node->construct);
        int lastLine = matte_tokenizer_current_line(graph->tokenizer);

        uint32_t i;
        uint32_t len = matte_array_get_size(root->paths);
        for(i = 0; i < len; ++i) {
            matteSyntaxGraphNode_t * tr = matte_array_at(root->paths, matteSyntaxGraphNode_t * , i);
            matteSyntaxGraphNode_t * out = matte_syntax_graph_walk_helper(graph, tr, node->construct, 1, error);
            if (*error) return NULL;
            if (out) {
                matte_table_clear(graph->tried);
                #ifdef MATTE_DEBUG__COMPILER
                    printf("  - PASSED initial construct node for path %s\n", matte_string_get_c_str(matte_array_at(root->pathNames, matteString_t *, i)));
                    fflush(stdout);

                #endif
                while((out = matte_syntax_graph_walk_helper(graph, out, node->construct, 0, error))) {
                    // The only way to validly finish a path
                    if (out && out->type == MATTE_SYNTAX_GRAPH_NODE__END) {
                        matte_table_clear(graph->tried);
                        #ifdef MATTE_DEBUG__COMPILER
                            level--;
                        #endif
                        return node->next;
                    }
                }
                if (*error) return NULL;
            } else {
                #ifdef MATTE_DEBUG__COMPILER
                    printf("  - FAILED initial construct node for path %s\n", matte_string_get_c_str(matte_array_at(root->pathNames, matteString_t *, i)));
                    fflush(stdout);

                #endif
            }
        }
        if (!silent) {
            matte_syntax_graph_print_error(graph, node, lastLine);
            *error = 1;
        }
        #ifdef MATTE_DEBUG__COMPILER
            level--;
        #endif
        return NULL;

        break;
      }


    }
    assert(!"should not reach here");
}

static int matte_syntax_graph_is_end(
    matteSyntaxGraphWalker_t * graph
) {
    return matte_tokenizer_is_end(graph->tokenizer);
}


// attempts to consume the current node and returns 
// the next node. If the node is not valid, NULL is returned.
static matteSyntaxGraphNode_t * matte_syntax_graph_walk(
    matteSyntaxGraphWalker_t * graph,
    matteSyntaxGraphNode_t * node,
    int constructID,
    int silent
) {
    int error = 0;
    return matte_syntax_graph_walk_helper(graph, node, constructID, silent, &error);
}

int matte_syntax_graph_continue(
    matteSyntaxGraphWalker_t * graph,
    int constructID
) {
    if (!matte_syntax_graph_is_construct(graph->graphsrc, constructID)) {
        matteString_t * str = matte_string_create_from_c_str("Internal error (no such constrctID)");
        if (graph->onError)
            graph->onError(str, 0, 0, graph->onErrorData);
        matte_string_destroy(str);
        return 0;
    }

    //matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(GRAPHSRC, constructID);
    matteSyntaxGraphNode_t topNode = {0};
    topNode.type = MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT; 
    topNode.construct = constructID;

    matteSyntaxGraphNode_t endNode = {0};
    endNode.type = MATTE_SYNTAX_GRAPH_NODE__END;

    topNode.next = &endNode;



    matteSyntaxGraphNode_t * next = &topNode;
    while((next = matte_syntax_graph_walk(graph, next, constructID, 0))) {
        // The only way to validly finish a path
        if (next && next->type == MATTE_SYNTAX_GRAPH_NODE__END) {
            return 1;
        }
        
        // or if we're out of characters
        if (matte_syntax_graph_is_end(graph)) {
            return 0;
        }
    }
    return 0;
}

/*
matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraphWalker_t * g) {
    return g->first;
}
*/


static void matte_token_print__helper(matteSyntaxGraphWalker_t * g, matteToken_t * t, matteString_t * str) {
    if (t == NULL) {
        matte_string_concat_printf(str, "line:%5d\tcol:%4d\t%-40s\t%s\n", 
            0, 0, "", ""
        );

        return;
    }
    matte_string_concat_printf(str, "line:%5d\tcol:%4d\t%-40s\t%s\n", 
        t->line,
        t->character,
        matte_string_get_c_str(matte_syntax_graph_get_token_name(g->graphsrc, t->ttype)),
        (!strcmp(matte_string_get_c_str((const matteString_t*)t->data), "\n") ? "" : matte_string_get_c_str((const matteString_t*)t->data))
    );


}

static matteString_t * matte_syntax_graph_print(matteSyntaxGraphWalker_t * g) {
    matteString_t * out = matte_string_create();
    matteToken_t * t = g->first; 
    while(t) {
        matte_token_print__helper(g, t, out);
        t = t->next;
    }

    return out;
}









///////////// compilation 

static matteArray_t * compile_expression(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
);


static matteFunctionBlock_t * compile_function_block(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * parent,
    matteArray_t * functions, 
    matteToken_t ** src
);

static void matte_syntax_graph_print_compile_error(
    matteSyntaxGraphWalker_t * graph,
    matteToken_t * t,
    const char * asciiMessage
) {
    matteString_t * message = matte_string_create_from_c_str("Compile Error: %s", asciiMessage);
    if (graph->onError)
        graph->onError(
            message,
            t->line,
            t->character,
            graph->onErrorData
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
          // because inlines are only made of expressions, there is no danger of 
          // irregular function-oriented tokens inherent to function definitions.
          case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_INLINE:
            *t = iter->next;
            return;
          case MATTE_TOKEN_FUNCTION_CONSTRUCTOR:
          case MATTE_TOKEN_FUNCTION_LISTEN:
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


static void function_block_destroy(matteFunctionBlock_t * t) {
    uint32_t i;
    uint32_t len;
    matte_array_destroy(t->instructions); // safe
    matte_array_destroy(t->captures); // safe

    len = matte_array_get_size(t->locals);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(t->locals, matteString_t *, i));        
    }
    matte_array_destroy(t->locals);
    matte_array_destroy(t->local_isConst);

    len = matte_array_get_size(t->args);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(t->args, matteString_t *, i));        
    }
    matte_array_destroy(t->args);

    len = matte_array_get_size(t->strings);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(t->strings, matteString_t *, i));        
    }
    matte_array_destroy(t->strings);


    len = matte_array_get_size(t->captureNames);
    for(i = 0; i < len; ++i) {
        matte_string_destroy(matte_array_at(t->captureNames, matteString_t *, i));        
    }
    matte_array_destroy(t->captureNames);
    matte_array_destroy(t->capture_isConst);
    if (t->typestrict_types) 
        matte_array_destroy(t->typestrict_types);
    matte_deallocate(t);

}

#include "MATTE_INSTRUCTIONS"


// returns 0xffffffff
static uint32_t get_local_referrable(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteToken_t * iter
) {
    uint32_t i;
    uint32_t len;


    //while(block) {
    len = matte_array_get_size(block->args);
    for(i = 0; i < len; ++i) {
        if (matte_string_test_eq((const matteString_t*)iter->data, matte_array_at(block->args, matteString_t *, i))) {
            return i+1;
        }
    }

    uint32_t offset = len+1;
    len = matte_array_get_size(block->locals);
    for(i = 0; i < len; ++i) {
        if (matte_string_test_eq((const matteString_t*)iter->data, matte_array_at(block->locals, matteString_t *, i))) {
            return offset+i;
        }
    }

    //block = block->parent;
    //}

    return 0xffffffff;
}

// returns whether the referrable for this block is constant.
// This will go through existing captures as well.
static int is_referrable_const(matteFunctionBlock_t * block, uint32_t referrable) {

    referrable--;    
    // overwriting local-face arguments is okay though!
    if (referrable < matte_array_get_size(block->args)) return 0;
    
    
    // function locals have their constance tracked.
    referrable -= matte_array_get_size(block->args);
    if (referrable < matte_array_get_size(block->locals)) {
        return matte_array_at(block->local_isConst, int, referrable);
    }
    
    // captures inherit their host function's constant state.
    referrable -= matte_array_get_size(block->locals);
    if (referrable < matte_array_get_size(block->capture_isConst))
        return matte_array_at(block->capture_isConst, int, referrable);

    #ifdef MATTE_DEBUG__COMPILER
        assert(!"Referrable was non-existent when its constness was checked...");
    #endif
    return 0;    
    
}


// retrieves a string interned to the function stub by ID to be used with nst ops.
// If non exists, the string is added.
static uint32_t function_intern_string(
    matteFunctionBlock_t * b,
    const matteString_t * str 
) {
    uint32_t i;
    uint32_t len = matte_array_get_size(b->strings);
    // look through interned strings 
    for(i = 0; i < len; ++i) {
        if (matte_string_test_eq(str, matte_array_at(b->strings, matteString_t *, i))) {
            return i;
        }
    }
    str = matte_string_clone(str);
    matte_array_push(b->strings, str);
    return len;
}



// attempts to return an array of additional instructions to push a variable onto the stack.
// returns 0 on failure
static matteArray_t * push_variable_name(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * inst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    matteFunctionBlock_t * blockOrig = block;
    uint32_t referrable = get_local_referrable(
        g,
        block,
        iter
    );
    if (referrable != 0xffffffff) {
        write_instruction__prf(inst, GET_LINE_OFFSET(block), referrable);
        *src = iter->next;
        return inst;
    } else {
        // first look through existing captures
        uint32_t i;
        uint32_t len;

        // special "_"
        if (!strcmp(matte_string_get_c_str((const matteString_t*)iter->data), "_")) {
            write_instruction__pip(inst, GET_LINE_OFFSET(block));
            *src = iter->next;
            return inst;                    
        }


        len = matte_array_get_size(block->captureNames);
        for(i = 0; i < len; ++i) {
            if (matte_string_test_eq((const matteString_t*)iter->data, matte_array_at(block->captureNames, matteString_t *, i))) {
                write_instruction__prf(inst, GET_LINE_OFFSET(block), i+1+matte_array_get_size(block->locals)+matte_array_get_size(block->args));
                *src = iter->next;
                return inst;                    
            }
        }

        // if not found, walk through function scopes 
        //      if found, append to existing captures
        matteFunctionBlock_t * blockSrc = block;
        block = block->parent;
        int isconst = 0;
        while(block) {

            len = matte_array_get_size(block->args);
            for(i = 0; i < len; ++i) {
                if (matte_string_test_eq((const matteString_t*)iter->data, matte_array_at(block->args, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = block->stubID;
                    capture.referrable = i+1;
                    write_instruction__prf(
                        inst, GET_LINE_OFFSET(blockSrc), 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matte_array_push(blockSrc->captures, capture);
                    matteString_t * str = matte_string_clone((const matteString_t*)iter->data);
                    matte_array_push(blockSrc->captureNames, str);
                    isconst = 0;
                    matte_array_push(blockSrc->capture_isConst, isconst);
                    *src = iter->next;
                    return inst;                    
                }
            }

            uint32_t offset = len+1;
            len = matte_array_get_size(block->locals);
            for(i = 0; i < len; ++i) {
                if (matte_string_test_eq((const matteString_t*)iter->data, matte_array_at(block->locals, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = block->stubID;
                    capture.referrable = i+offset;
                    write_instruction__prf(
                        inst, GET_LINE_OFFSET(blockSrc), 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matte_array_push(blockSrc->captures, capture);
                    matteString_t * str = matte_string_clone((const matteString_t*)iter->data);
                    matte_array_push(blockSrc->captureNames, str);
                    isconst = is_referrable_const(block, i + 1 + matte_array_get_size(block->args));
                    matte_array_push(blockSrc->capture_isConst, isconst);

                    *src = iter->next;
                    return inst;                    

                }
            }
            
            if (matte_array_get_size(blockSrc->captures) > FUNCTION_CAPTURES_MAX)
                matte_syntax_graph_print_compile_error(g, iter, "Function exceeds count limit of lexically captured variables.");
            
            
            block = block->parent;
        }
    }

    if (OPTION__NAMED_REFERENCES) {
        uint32_t i = function_intern_string(blockOrig, (const matteString_t*)iter->data);
        write_instruction__pnr(inst, block ? GET_LINE_OFFSET(block) : 0, i);
        *src = iter->next;
        return inst;
    } else {
        matteString_t * m = matte_string_create_from_c_str("Undefined variable '%s'", matte_string_get_c_str((const matteString_t*)iter->data));
        matte_syntax_graph_print_compile_error(g, iter, matte_string_get_c_str(m));
        matte_string_destroy(m);

        matte_array_destroy(inst);
        return NULL;
    }
}


matteOperator_t string_to_operator(const matteString_t * s, matteTokenType_t hint) {
    int opopcode0 = matte_string_get_char(s, 0);
    int opopcode1 = matte_string_get_char(s, 1);
    switch(opopcode0) {
        case '+': 
        return MATTE_OPERATOR_ADD; break;

        case '-': 
        switch(opopcode1) {
            case '>': return MATTE_OPERATOR_POINT; break;
        }
        return (hint == MATTE_TOKEN_GENERAL_OPERATOR1 ? MATTE_OPERATOR_NEGATE : MATTE_OPERATOR_SUB); 
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
            case '>': return MATTE_OPERATOR_SHIFT_RIGHT; break;
            case '=': return MATTE_OPERATOR_GREATEREQ; break;
            default:;
        }
        return MATTE_OPERATOR_GREATER;
        break;

        case '=':
        switch(opopcode1) {
            case '>': return MATTE_OPERATOR_TYPESPEC; break;
            case '=': return MATTE_OPERATOR_EQ; break;
            default:;
        }
        break;

        case '~': return MATTE_OPERATOR_BITWISE_NOT; break;
        break;

        case '#': return MATTE_OPERATOR_POUND; break;
        case '?': return MATTE_OPERATOR_TERNARY; break;
        case '^': return MATTE_OPERATOR_CARET; break;
        case '%': return MATTE_OPERATOR_MODULO; break;


        break;
        default:;
    }
    return MATTE_OPERATOR_ERROR;
}


// pushes all instructions within the array and destroys the array


static void merge_instructions(
    matteArray_t * m,
    matteArray_t * arr
) {
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    for(i = 0; i < len; ++i) {
        matte_array_push(m, matte_array_at(arr, matteBytecodeStubInstruction_t, i));
    }
    matte_array_destroy(arr);
}

static void merge_instructions_rev(
    matteArray_t * m,
    matteArray_t * arr
) {
    matte_array_insert_n(
        m,
        0,
        matte_array_get_data(arr),
        matte_array_get_size(arr)
    );
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
    int postOp;

    // starts at 0 for a same-level expression
    int appearanceID;

    // precedence level
    int precedence;

    // source line number for the start of the value;
    int line;

    // array of instructions that, when run, push the vaue in question
    matteArray_t * value; 

    // next value linked in the 2-operand operation, if any.
    // PLEASE NOTE: these are the COMPUTATIONALLY SURROUNDED nodes
    // nodes are not computed from left to right but are resorted and 
    // collapse neighboring nodes in order of precedence
    matteExpressionNode_t * next;

};

// more or less C!
//
// Though.... i have a vague feeling that evil lurks here...
// an old, inherent evil...
    
#define POST_OP_SYMBOLIC__ASSIGN_REFERRABLE 10000
#define POST_OP_SYMBOLIC__ASSIGN_MEMBER     20000
static int op_to_precedence(int op) {
    switch(op) {
      case MATTE_OPERATOR_POUND: // # 1 operand
        return 0;
      case MATTE_OPERATOR_POINT: // -> 2 operands
        return 1;


      case MATTE_OPERATOR_NOT:
      case MATTE_OPERATOR_BITWISE_NOT:
      case MATTE_OPERATOR_NEGATE:
      case MATTE_OPERATOR_POW: // ** 2 operands
        return 2;

      case MATTE_OPERATOR_DIV:
      case MATTE_OPERATOR_MULT:
      case MATTE_OPERATOR_MODULO: // % 2 operands
        return 3; 

      case MATTE_OPERATOR_ADD:
      case MATTE_OPERATOR_SUB:
        return 4;


      case MATTE_OPERATOR_SHIFT_LEFT: // << 2 operands
      case MATTE_OPERATOR_SHIFT_RIGHT: // >> 2 operands
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
      case MATTE_OPERATOR_TRANSFORM: // <> 2 operands
      case MATTE_OPERATOR_CARET: // ^ 2 operands
        return 13;



    }
    return 999;
}

static matteExpressionNode_t * new_expression_node(
    int preOp,
    int postOp,
    int appearanceID,
    int lineNumber,
    // xfer ownership
    matteArray_t * value
) {
    matteExpressionNode_t * out = (matteExpressionNode_t*)matte_allocate(sizeof(matteExpressionNode_t));
    out->preOp = preOp;
    out->postOp = postOp;
    out->appearanceID = appearanceID;
    out->precedence = op_to_precedence(out->postOp);
    out->value = value;
    out->line = lineNumber;
    return out;
}



static int expression_node_sort__comparator(
    const void * aSrc,
    const void * bSrc
) {
    matteExpressionNode_t * a = *(matteExpressionNode_t**)aSrc;
    matteExpressionNode_t * b = *(matteExpressionNode_t**)bSrc;

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
static matteArray_t * compile_base_value(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src,
    int * lvalue
) {
    matteToken_t * iter = *src;
    matteArray_t * inst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

    switch(iter->ttype) {
      case MATTE_TOKEN_LITERAL_BOOLEAN:
        write_instruction__nbl(inst, GET_LINE_OFFSET(block), !strcmp(matte_string_get_c_str((matteString_t*)iter->data), "true"));
        *src = iter->next;
        return inst;

      case MATTE_TOKEN_LITERAL_NUMBER: {
        double val = 0.0;
        uint32_t valh = 0;
        if (sscanf(matte_string_get_c_str((matteString_t*)iter->data), "%lf", &val)) {
            write_instruction__nnm(inst, GET_LINE_OFFSET(block), val); 
        } else if (sscanf(matte_string_get_c_str((matteString_t*)iter->data), "%x", &valh)) {            
            write_instruction__nnm(inst, GET_LINE_OFFSET(block), valh); 
        }
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_LITERAL_EMPTY: {
        write_instruction__nem(inst, GET_LINE_OFFSET(block));
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_LITERAL_STRING: {
        matteString_t * str = (matteString_t*)iter->data;
        uint32_t strl = matte_string_get_length(str);
        if(strl == 2) {
            matteString_t * empty = matte_string_create();
            write_instruction__nst(inst, GET_LINE_OFFSET(block), function_intern_string(block, empty));
            matte_string_destroy(empty);
        } else {        
            write_instruction__nst(inst, GET_LINE_OFFSET(block), function_intern_string(block, matte_string_get_substr((matteString_t*)iter->data, 1, strl-2)));
        }
        *src = iter->next;
        return inst;
      }


      case MATTE_TOKEN_EXTERNAL_PRINT: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_PRINT);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_SEND: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_SEND);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_BREAKPOINT: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_BREAKPOINT);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_ERROR: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_ERROR);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_GETEXTERNALFUNCTION);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORT: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_IMPORT);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORTMODULE: {
        write_instruction__ext(inst, GET_LINE_OFFSET(block), MATTE_EXT_CALL_IMPORTMODULE);
        *src = iter->next;
        return inst;
      }

      
      case MATTE_TOKEN_EXTERNAL_TYPEEMPTY: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 0);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 1);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPENUMBER: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 2);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPESTRING: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 3);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEOBJECT: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 4);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEFUNCTION: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 5);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPETYPE: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 6);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEANY: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 7);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPENULLABLE: {
        write_instruction__pto(inst, GET_LINE_OFFSET(block), 8);
        *src = iter->next;
        return inst;
      } 




      case MATTE_TOKEN_VARIABLE_NAME: {
        matte_array_destroy(inst);
        *lvalue = 1;
        return push_variable_name(
            g, 
            block,
            src
        );
      }


      // array literal
      case MATTE_TOKEN_OBJECT_ARRAY_START: {
        write_instruction__nob(inst, GET_LINE_OFFSET(block));
        iter = iter->next;
        
        while(iter->ttype != MATTE_TOKEN_OBJECT_ARRAY_END) {
            matteToken_t * spread = NULL;
            if (iter->ttype == MATTE_TOKEN_OBJECT_SPREAD) {                
                spread = iter;
                iter = iter->next;
            }
        
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
            if (iter->ttype != MATTE_TOKEN_OBJECT_ARRAY_END) 
                iter = iter->next; // skip ,

            
            merge_instructions(inst, expInst);
            if (spread) {
                write_instruction__spa(inst, iter->line - spread->line);            
            } else {
                write_instruction__caa(inst, GET_LINE_OFFSET(block));            
            }
        }
        *src = iter->next; // skip ]
        return inst;
        break;
      }



      // object literal
      case MATTE_TOKEN_OBJECT_LITERAL_BEGIN: {
        write_instruction__nob(inst, GET_LINE_OFFSET(block));
        iter = iter->next;
        while(iter->ttype != MATTE_TOKEN_OBJECT_LITERAL_END) {
            // key : value,
            // OR
            // key :: function
            // OR
            // ...value
            matteToken_t * spread = NULL;
            if (iter->ttype == MATTE_TOKEN_OBJECT_SPREAD) {
                spread = iter;
                iter = iter->next;

            // special case: variable name is treated like a string for convenience
            } else if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME &&
                (iter->next->next->ttype == MATTE_TOKEN_GENERAL_SPECIFIER ||
                 iter->next->next->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR)
            ) {
                
                write_instruction__nst(inst, GET_LINE_OFFSET(block), function_intern_string(block, (matteString_t*)iter->data));
                iter = iter->next; // skip name
                iter = iter->next; // skip marker
                


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

            if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER) {
                iter = iter->next; // skip : 
            }// else its part of the function constructor: LEAVE IT.



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
            if (spread) {
                write_instruction__spo(inst, spread->line - block->startingLine);                        
            } else {
                write_instruction__cas(inst, GET_LINE_OFFSET(block));                        
            }


            if (iter->ttype == MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR)
                iter = iter->next;                
        }
        *src = iter->next; // skip } 
        return inst;
      }




      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: { 
        matteArray_t * arr = (matteArray_t*)iter->data; // the sneaky in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }


      case MATTE_TOKEN_EXTERNAL_GATE: { 
        matteArray_t * arr = (matteArray_t*)iter->data; // the sneaky II in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_EXTERNAL_MATCH: { 
        matteArray_t * arr = (matteArray_t*)iter->data; // the sneaky III in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_FUNCTION_LISTEN: { 
        matteArray_t * arr = (matteArray_t*)iter->data; // the sneaky VI in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }


      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_WITH_SPECIFIER:
        assert(!"Internal compiler error: Constructor with specifier should be replaced with a normal function constructor in parent context.");
        break;
              
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR:  {

        matteFunctionBlock_t * fn = (matteFunctionBlock_t*)iter->data; // the sneaky IV in action....
        if (fn->isEmpty) {
            write_instruction__nef(
                inst, 
                GET_LINE_OFFSET(block)
            );
            
        } else if (fn->typestrict) {
            merge_instructions(inst, matte_array_clone(fn->typestrict_types));
            write_instruction__sfs(
                inst, 
                GET_LINE_OFFSET(block),
                fn->typestrict
            );
            write_instruction__nfn(
                inst, 
                GET_LINE_OFFSET(block),
                fn->stubID
            );
        
        } else {
            write_instruction__nfn(
                inst, 
                GET_LINE_OFFSET(block),
                fn->stubID
            );
        }
        if (fn->isDashed) {
            write_instruction__cal(
                inst, 
                GET_LINE_OFFSET(block),
                0
            );        
        }
        *src = iter->next;
        return inst;
      }
      default:;
        if (token_is_assignment_derived(iter->ttype)) {
            matteArray_t * arr = (matteArray_t*)iter->data; // the sneaky V in action....
            merge_instructions(inst, matte_array_clone(arr));
            *src = iter->next;
            return inst;
        }              
        
    }

    matte_syntax_graph_print_compile_error(g, iter, "Unrecognized value token (internal error)");
    matte_array_destroy(inst);
    return NULL;               

}

static int query_name_to_index(const matteString_t * str) {
    const char * st = matte_string_get_c_str(str);
    if (!strcmp(st, "cos"))   return MATTE_QUERY__COS;
    if (!strcmp(st, "sin"))   return MATTE_QUERY__SIN;
    if (!strcmp(st, "tan"))   return MATTE_QUERY__TAN;
    if (!strcmp(st, "acos"))  return MATTE_QUERY__ACOS;
    if (!strcmp(st, "asin"))  return MATTE_QUERY__ASIN;
    if (!strcmp(st, "atan"))  return MATTE_QUERY__ATAN;
    if (!strcmp(st, "atan2")) return MATTE_QUERY__ATAN2;
    if (!strcmp(st, "sqrt"))  return MATTE_QUERY__SQRT;
    if (!strcmp(st, "abs"))   return MATTE_QUERY__ABS;
    if (!strcmp(st, "isNaN")) return MATTE_QUERY__ISNAN;
    if (!strcmp(st, "floor")) return MATTE_QUERY__FLOOR;
    if (!strcmp(st, "ceil")) return MATTE_QUERY__CEIL;
    if (!strcmp(st, "round")) return MATTE_QUERY__ROUND;
    if (!strcmp(st, "asRadians")) return MATTE_QUERY__RADIANS;
    if (!strcmp(st, "asDegrees")) return MATTE_QUERY__DEGREES;
    if (!strcmp(st, "removeChar")) return MATTE_QUERY__REMOVECHAR;
    if (!strcmp(st, "substr")) return MATTE_QUERY__SUBSTR;
    if (!strcmp(st, "split")) return MATTE_QUERY__SPLIT;
    if (!strcmp(st, "scan")) return MATTE_QUERY__SCAN;
    if (!strcmp(st, "length")) return MATTE_QUERY__LENGTH;
    if (!strcmp(st, "search")) return MATTE_QUERY__SEARCH;
    if (!strcmp(st, "searchAll")) return MATTE_QUERY__SEARCH_ALL;
    if (!strcmp(st, "contains")) return MATTE_QUERY__CONTAINS;
    if (!strcmp(st, "replace")) return MATTE_QUERY__REPLACE;
    if (!strcmp(st, "format")) return MATTE_QUERY__FORMAT;
    if (!strcmp(st, "count")) return MATTE_QUERY__COUNT;
    if (!strcmp(st, "charCodeAt")) return MATTE_QUERY__CHARCODEAT;
    if (!strcmp(st, "charAt")) return MATTE_QUERY__CHARAT;
    if (!strcmp(st, "setCharCodeAt")) return MATTE_QUERY__SETCHARCODEAT;
    if (!strcmp(st, "setCharAt")) return MATTE_QUERY__SETCHARAT;
    if (!strcmp(st, "keycount")) return MATTE_QUERY__KEYCOUNT;
    if (!strcmp(st, "size")) return MATTE_QUERY__SIZE;
    if (!strcmp(st, "setSize")) return MATTE_QUERY__SETSIZE;
    if (!strcmp(st, "keys")) return MATTE_QUERY__KEYS;
    if (!strcmp(st, "values")) return MATTE_QUERY__VALUES;
    if (!strcmp(st, "push")) return MATTE_QUERY__PUSH;
    if (!strcmp(st, "pop")) return MATTE_QUERY__POP;
    if (!strcmp(st, "insert")) return MATTE_QUERY__INSERT;
    if (!strcmp(st, "remove")) return MATTE_QUERY__REMOVE;
    if (!strcmp(st, "setAttributes")) return MATTE_QUERY__SETATTRIBUTES;
    if (!strcmp(st, "attributes")) return MATTE_QUERY__ATTRIBUTES;
    if (!strcmp(st, "sort")) return MATTE_QUERY__SORT;
    if (!strcmp(st, "subset")) return MATTE_QUERY__SUBSET;
    if (!strcmp(st, "filter")) return MATTE_QUERY__FILTER;
    if (!strcmp(st, "findIndex")) return MATTE_QUERY__FINDINDEX;
    if (!strcmp(st, "findIndexCondition")) return MATTE_QUERY__FINDINDEXCONDITION;
    if (!strcmp(st, "isa")) return MATTE_QUERY__ISA;
    if (!strcmp(st, "map")) return MATTE_QUERY__MAP;
    if (!strcmp(st, "reduce")) return MATTE_QUERY__REDUCE;
    if (!strcmp(st, "any")) return MATTE_QUERY__ANY;
    if (!strcmp(st, "all")) return MATTE_QUERY__ALL;
    if (!strcmp(st, "foreach")) return MATTE_QUERY__FOREACH;
    if (!strcmp(st, "type")) return MATTE_QUERY__TYPE;
    if (!strcmp(st, "setIsInterface")) return MATTE_QUERY__SET_IS_INTERFACE;
    return -1;
}

static matteArray_t * compile_function_call(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
);
static matteArray_t * compile_value(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src,
    int * lvalue
) {
    *lvalue = 0;
    matteArray_t * inst = compile_base_value(
        g, 
        block,
        functions, 
        src,
        lvalue
    );
    matteToken_t * iter = *src;
    if (!inst) return NULL;

    // any value can have "special" postfix operations 
    // applied to it repeatedly

    for(;;) {
        // any value can be computed to an object, which means 
        // any value should syntactically allow for the dot accessor:
        if (iter->ttype == MATTE_TOKEN_OBJECT_ACCESSOR_DOT) {            
            iter = iter->next; // skip .
            // if so, the next will be a variable name
            if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME) {
                write_instruction__nst(inst, GET_LINE_OFFSET(block), function_intern_string(block, (matteString_t*) iter->data));
                write_instruction__olk(inst, GET_LINE_OFFSET(block), 0);
                *lvalue = 1;
                iter = iter->next;
            } else if (iter->ttype == MATTE_TOKEN_ASSIGNMENT) {
                matteArray_t * arr = (matteArray_t *)iter->data; // the sneaky V in action....
                if (!arr) {
                    matte_array_destroy(inst);
                    return NULL;
                }
                // we dont want to lose the previous data, so manually remove;
                iter->data = NULL;
                iter->dataType = -1;
                iter = iter->next; // skip =
                merge_instructions(inst, arr); // push argument
                write_instruction__oas(inst, GET_LINE_OFFSET(block));
            } else {                
            
                matte_syntax_graph_print_compile_error(g, iter, "Dot accessor expects variable name-style member to access as a string.");
                matte_array_destroy(inst);
                return NULL;               
            }
        } else if (iter->ttype == MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START) {            
            iter = iter->next; // skip [
            matteArray_t * exp = compile_expression(g, block, functions, &iter);
            if (!exp) {
                matte_array_destroy(inst);
                return NULL;               
            }
            merge_instructions(inst, exp); // push argument
            write_instruction__olk(inst, GET_LINE_OFFSET(block), 1);
            iter = iter->next; // skip ]        
        // function calls are kind of like special operators
        // here, they use the compiled value above as the function object 
        // and have computed arguments passed to it
        } else if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_BEGIN) {
            matteArray_t * funcInst = compile_function_call(g, block, functions, &iter);
            if (funcInst) {
                merge_instructions(inst, funcInst);
            } else {
                matte_array_destroy(inst);
                return NULL;               
            }
        // queries return a value of some kind. Sometimes theyre functions, sometimes not.
        } else if (iter->ttype == MATTE_TOKEN_QUERY_OPERATOR) {
            iter = iter->next;
            int index = query_name_to_index((matteString_t*)iter->data);
            
            if (index == -1) {
                matteString_t * m = matte_string_create_from_c_str("Unrecognized query name '%s'", matte_string_get_c_str((matteString_t*)iter->data));            
                matte_syntax_graph_print_compile_error(g, iter, matte_string_get_c_str(m));
                matte_string_destroy(m);
                matte_array_destroy(inst);
                return NULL;               
            }
            write_instruction__qry(
                inst,
                GET_LINE_OFFSET(block),
                index
            );
            iter = iter->next;
        }

        else {
            // no more post ops
            break;
        }
    }
    *src = iter;
    return inst;
}


// Pushes instructions required to call a function
// Starts at MATTE_TOKEN_FUNCTION_ARG_BEGIN and consumes the FUNCTION_ARG_END
static matteArray_t * compile_function_call(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    uint32_t argCount = 0;
    matteToken_t * iter = (*src)->next; // skip (
    matteArray_t * inst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    int isVarArg = 0;
    while(iter->ttype != MATTE_TOKEN_FUNCTION_ARG_END) {
        matteArray_t * exp;

        // first check if var arg call
        if (iter->ttype == MATTE_TOKEN_VARARG) {
            isVarArg = 1;
            iter = iter->next; // skip *    
            
            // compile object to be the vararg
            exp = compile_expression(g, block, functions, &iter);
            merge_instructions(inst, exp); // push argument
            break;
        }
        
        if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER ||
            iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {

            if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER)
                iter = iter->next; // skip :    
            
           
            // add a dummy blank string as the argname. This tells 
            // the runtime to treat it as an automatic argument.
            matteString_t * blank = matte_string_create();
            uint32_t i = function_intern_string(block, blank);
            matte_string_destroy(blank);
           
            exp = compile_expression(g, block, functions, &iter);
            if (exp) {
                merge_instructions(inst, exp); // push argument
                write_instruction__nst(inst, iter->line - block->startingLine, i);
            } else {
                goto L_FAIL;
            }
            break;        
        }
    
        // parse out the parameters
        uint32_t i = function_intern_string(block, (matteString_t*)iter->data);
        uint32_t nameLineNum = iter->line;

        if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME && !strcmp("$", matte_string_get_c_str((matteString_t*)iter->data))) {
            matte_syntax_graph_print_compile_error(g, iter, "Cannot bind as argument special dynamic binding character $.");
            goto L_FAIL;            
        }

        if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME && !strcmp("_", matte_string_get_c_str((matteString_t*)iter->data))) {
            matte_syntax_graph_print_compile_error(g, iter, "Cannot bind as argument special interface private accessor _.");
            goto L_FAIL;            
        }


        // usual case -> "name: expression"
        if (iter->next->ttype == MATTE_TOKEN_GENERAL_SPECIFIER) {           
            iter = iter->next->next; // skip :    
            exp = compile_expression(g, block, functions, &iter);

        // convenient case -> function constructors, omitting extra ":"
        } else if (iter->next->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {
            iter = iter->next; // skip just name
            exp = compile_expression(g, block, functions, &iter);


        // convenient case -> "name" (which is also the expression)
        } else {
            int isLvalue; // nused
            exp = compile_value(g, block, functions, &iter, &isLvalue);        
        }
        if (!exp) {
            goto L_FAIL;
        }
        merge_instructions(inst, exp); // push argument
        write_instruction__nst(inst, nameLineNum - block->startingLine, i);
        argCount++;
        if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_SEPARATOR)
            iter = iter->next; // skip ,
    }

    if (isVarArg)
        write_instruction__clv(inst, GET_LINE_OFFSET(block), argCount);
    else
        write_instruction__cal(inst, GET_LINE_OFFSET(block), argCount);
    *src = iter->next; // skip )
    return inst;

  L_FAIL:;
    matte_array_destroy(inst);
    return NULL;
}


static matteToken_t * ff_skip_inner_arg(matteToken_t * iter) {
    iter = iter->next;
    while(iter && iter->ttype != MATTE_TOKEN_FUNCTION_ARG_END) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_BEGIN) {
            iter = ff_skip_inner_arg(iter);
        }
        iter = iter->next;
    }
    return iter;
}

static matteToken_t * ff_skip_inner_bracket(matteToken_t * iter) {
    iter = iter->next;
    while(iter && iter->ttype != MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_END) {
        if (iter->ttype == MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START) {
            iter = ff_skip_inner_bracket(iter);
        }
        iter = iter->next;
    }
    return iter;
}



static matteToken_t * ff_skip_inner_object_static(matteToken_t * iter) {
    iter = iter->next;
    while(iter && iter->ttype != MATTE_TOKEN_OBJECT_LITERAL_END) {
        if (iter->ttype == MATTE_TOKEN_OBJECT_LITERAL_BEGIN) {
            iter = ff_skip_inner_object_static(iter);
        }
        iter = iter->next;
    }
    return iter;
}

static matteToken_t * ff_skip_inner_array_static(matteToken_t * iter) {
    iter = iter->next;
    while(iter && iter->ttype != MATTE_TOKEN_OBJECT_ARRAY_END) {
        if (iter->ttype == MATTE_TOKEN_OBJECT_ARRAY_START) {
            iter = ff_skip_inner_array_static(iter);
        }
        iter = iter->next;
    }
    return iter;
}

// 
// Listen is simply: 
// push fn 
// push fn 
// lst
static matteArray_t * compile_listen(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * instOut = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    iter = iter->next; // skip [::]

    matteFunctionBlock_t * b = compile_function_block(g, block, functions, &iter);
    if (!b) {
        goto L_FAIL;
    }

    
    matte_array_push(functions, b);
    write_instruction__nfn(
        instOut, 
        GET_LINE_OFFSET(block),
        b->stubID
    );

    if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER) {
        iter = iter->next; // skip :

        matteArray_t * inst = compile_expression(g, block, functions, &iter);
        if (!inst) {
            goto L_FAIL;
        }
        merge_instructions(instOut, inst);
        write_instruction__lst(instOut, GET_LINE_OFFSET(block));
    } else {
        write_instruction__nem(instOut, GET_LINE_OFFSET(block));
        write_instruction__lst(instOut, GET_LINE_OFFSET(block));
    }
    *src = iter;
    return instOut;

  L_FAIL:
    matte_array_destroy(instOut);
    return NULL;
}


// compiles the match statement. 
// its not too bad! the general flow is this:
// 1. push initial expression result
// 2. For each match condition expression:
//
//       cpy
//       [condition expression]
//       opr ==
//       skp 1
//       asp [location of corresponding result expression]
//
//  3. Once all conditions are writte, THEN all the results are 
//     written.
//
//       pop
//       [result expression]
//       asp [location of end of match]
//
//     IF the default expression exists, its expression is 
//     THE FIRST expression in the list. If the default expression doesnt exist 
//     it is replaced with this expression:
//
//        pop
//        nem
//        asp [location of end of match]
//      
// Because of this organization, MATCH OFFSETS MUST BE 
// HANDLED VERY, VERY CAREFULLY. Blease....
static matteArray_t * compile_match(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * instOut = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    matteArray_t * defaultExpression = NULL;
    uint32_t pivotDistance = 0;
    uint32_t endDistance = 0;
    uint32_t defaultLine = iter->line;
    typedef struct {
        // compiled condition instructions
        matteArray_t * result;
        // The index of the match result
        uint32_t offset;
        uint32_t line;
    } matchresult;

    // array of matteArray_t * of instructions
    matteArray_t * resultExpressions = matte_array_create(sizeof(matchresult));
    typedef struct {
        // compiled condition instructions
        matteArray_t * condition;
        // The index of the match result
        uint32_t matchResult;
        uint32_t line;
    } matchcondition;

    matteArray_t * conditionExpressions = matte_array_create(sizeof(matchcondition));
    uint32_t i;



    iter = iter->next; // skip "match"
    iter = iter->next; // skip "("
    matteArray_t * inst = compile_expression(g, block, functions, &iter);
    if (!inst) {
        goto L_FAIL;
    }
    // no matter what, the initial expression is pushed
    merge_instructions(instOut, inst);


    iter = iter->next; // skip ")"





    while(iter->ttype != MATTE_TOKEN_MATCH_END) {
        iter = iter->next; // skip "{" or ",";
        if (iter->ttype == MATTE_TOKEN_MATCH_DEFAULT) {
            iter = iter->next; // skip default
            iter = iter->next; // skip :
            if (defaultExpression) {
                matte_syntax_graph_print_compile_error(g, iter, "Duplicate 'default' case in match.");
                goto L_FAIL;
            }
            defaultLine = iter->line;
            defaultExpression = compile_expression(g, block, functions, &iter);
            if (!defaultExpression) {
                goto L_FAIL;
            }            
        } else {
            while(iter->ttype != MATTE_TOKEN_IMPLICATION_END) {
                iter = iter->next; // skip ( or ,

                matchcondition c;
                c.line = iter->line;
                matteArray_t * inst = compile_expression(g, block, functions, &iter);
                if (!inst) {
                    goto L_FAIL;
                }
                c.condition = inst;
                c.matchResult = matte_array_get_size(resultExpressions);
                matte_array_push(conditionExpressions, c);
            }
            matchresult result;
            iter = iter->next; // skip the )
                
            if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER)
                iter = iter->next; // skip the :
            result.line = iter->line;
            matteArray_t * inst = compile_expression(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }    
            result.result = inst;
            matte_array_push(resultExpressions, result);
        }

    }
    iter = iter->next; // skip "}";


    // now that we have everything within it compiled,
    // its time to assemble the table.

    // distance in instructions from the "pivot", pivot 
    // is the start of the results (always starts with)
    // either default or the default placeholder
    pivotDistance = 0;
    for(i = 0; i < matte_array_get_size(conditionExpressions); ++i) {
        pivotDistance += 4+matte_array_get_size(matte_array_at(conditionExpressions, matchcondition, i).condition);
    }

    endDistance = 0;
    if (defaultExpression) {
        endDistance += 2 + matte_array_get_size(defaultExpression);
    } else {
        endDistance += 3;
    }
    for(i = 0; i < matte_array_get_size(resultExpressions); ++i) {
        matte_array_at(resultExpressions, matchresult, i).offset = endDistance;
        endDistance += 2 + matte_array_get_size(matte_array_at(resultExpressions, matchresult, i).result);
    }


    for(i = 0; i < matte_array_get_size(conditionExpressions); ++i) {
        matchcondition c = matte_array_at(conditionExpressions, matchcondition, i);
        matchresult r = matte_array_at(resultExpressions, matchresult, c.matchResult);
        uint32_t len = matte_array_get_size(c.condition);
        write_instruction__cpy(instOut, c.line - block->startingLine);
        merge_instructions(instOut, c.condition);
        write_instruction__opr(instOut, c.line - block->startingLine, MATTE_OPERATOR_EQ);
        write_instruction__skp_insert(instOut, c.line, 1);
        pivotDistance -= 4+len;
        write_instruction__asp(instOut, c.line - block->startingLine, pivotDistance+r.offset);
    }
    assert(pivotDistance == 0);
    matte_array_destroy(conditionExpressions);

    write_instruction__pop(instOut, defaultLine - block->startingLine, 1); endDistance--;
    if (defaultExpression) {
        endDistance-=matte_array_get_size(defaultExpression);
        merge_instructions(instOut, defaultExpression);
    } else {
        write_instruction__nem(instOut, defaultLine - block->startingLine);endDistance--;
    }
    write_instruction__asp(instOut, defaultLine - block->startingLine, endDistance-1);endDistance--;


    for(i = 0; i < matte_array_get_size(resultExpressions); ++i) {
        matchresult r = matte_array_at(resultExpressions, matchresult, i);
        uint32_t len = matte_array_get_size(r.result);
        write_instruction__pop(instOut, defaultLine, 1);
        merge_instructions(instOut, r.result);
        endDistance -= len + 2;
        write_instruction__asp(instOut, defaultLine - block->startingLine, endDistance);
    }
    matte_array_destroy(resultExpressions);
    assert(endDistance == 0);

    *src = iter;
    return instOut;
  L_FAIL:
    matte_array_destroy(instOut);
    if (defaultExpression) matte_array_destroy(defaultExpression);
    for(i = 0; i < matte_array_get_size(resultExpressions); ++i) {
        matte_array_destroy(matte_array_at(resultExpressions, matchresult, i).result);
    }
    matte_array_destroy(resultExpressions);
    for(i = 0; i < matte_array_get_size(conditionExpressions); ++i) {
        matte_array_destroy(matte_array_at(conditionExpressions, matchcondition, i).condition);
    }
    matte_array_destroy(conditionExpressions);

    return NULL;
}



static int assignment_token_to_op_index(matteTokenType_t t) {
    switch(t) {
      case MATTE_TOKEN_ASSIGNMENT_ADD: return 1;
      case MATTE_TOKEN_ASSIGNMENT_SUB: return 2;
      case MATTE_TOKEN_ASSIGNMENT_MULT: return 3;
      case MATTE_TOKEN_ASSIGNMENT_DIV: return 4;
      case MATTE_TOKEN_ASSIGNMENT_MOD: return 5;
      case MATTE_TOKEN_ASSIGNMENT_POW: return 6;
      case MATTE_TOKEN_ASSIGNMENT_AND: return 7;
      case MATTE_TOKEN_ASSIGNMENT_OR:  return 8;
      case MATTE_TOKEN_ASSIGNMENT_XOR: return 9;
      case MATTE_TOKEN_ASSIGNMENT_BLEFT: return 10;
      case MATTE_TOKEN_ASSIGNMENT_BRIGHT: return 11;
      default:;
    }
    return 0;
}   

// Returns an array of instructions that, when computed in order,
// produce exactly ONE value on the stack (the evaluation of the expression).
// returns NULL on failure. Expected to report errors 
// when encountered.
// The MATTE_TOKEN_MARKER_EXPRESSION_END token is consumed.
static matteArray_t * compile_expression(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteArray_t * outInst = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    matteArray_t * nodes = matte_array_create(sizeof(matteExpressionNode_t *));
    int appearanceID = 0;
    int lvalue;
    int hasandor;
    uint32_t len;
    uint32_t i, si;
    matteExpressionNode_t * prev = NULL;
    matteToken_t * last;

    // parentheticals should be evaluated first.
    // also!! functions are always constructed as part of 
    // expressions. In Matte function constructors <-> definitions
    while(iter && iter->ttype != MATTE_TOKEN_MARKER_EXPRESSION_END) {
        switch(iter->ttype) {
          case MATTE_TOKEN_EXTERNAL_MATCH: {
            matteToken_t * start = iter;
            matteArray_t * inst = compile_match(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }
            matteToken_t * end = iter;        
            matte_token_new_data(start, inst, MATTE_TOKEN_DATA_TYPE__ARRAY_INST);    
            

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

          case MATTE_TOKEN_FUNCTION_LISTEN: {
            matteToken_t * start = iter;
            matteArray_t * inst = compile_listen(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }
            matteToken_t * end = iter;            
            matte_token_new_data(start, inst, MATTE_TOKEN_DATA_TYPE__ARRAY_INST);    
            

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

          // gate expressions mimic ternary operators in c-likes <3
          case MATTE_TOKEN_EXTERNAL_GATE: {
            matteToken_t * start = iter;
            iter = iter->next; // skip "gate"
            iter = iter->next; // skip "("
            matteArray_t * inst = compile_expression(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }

            iter = iter->next; // skip ")"
            matteArray_t * iftrue;
            matteArray_t * iffalse;

            // true bit is always there
            iftrue = compile_expression(g, block, functions, &iter);
            if (!iftrue) {
                goto L_FAIL;
            }


            // only true, else empty
            if (iter->ttype == MATTE_TOKEN_GATE_RETURN)  {
                iter = iter->next; // skip ":"

                iffalse = compile_expression(g, block, functions, &iter);
                if (!iffalse) {
                    goto L_FAIL;
                }
            } else {
                iffalse = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
                write_instruction__nem(iffalse, start->line - block->startingLine);
            }




            matteToken_t * end = iter;
            write_instruction__skp_insert(inst, start->line - block->startingLine, matte_array_get_size(iftrue)+1); // +1 for the ASP
            merge_instructions(inst, iftrue);
            write_instruction__asp(inst, start->line - block->startingLine, matte_array_get_size(iffalse));
            merge_instructions(inst, iffalse);

            
            // to reference this expressions value, the string is REPLACED 
            // with a pointer to the array.
            
            matte_token_new_data(start, inst, MATTE_TOKEN_DATA_TYPE__ARRAY_INST);    
            

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

          case MATTE_TOKEN_OBJECT_LITERAL_BEGIN: {
            iter = ff_skip_inner_object_static(iter);
            if (!iter) {
                matte_syntax_graph_print_compile_error(g, iter, "Object literal missing end '}'");
                goto L_FAIL;
            }
            break;
          }
          case MATTE_TOKEN_OBJECT_ARRAY_START: {
            iter = ff_skip_inner_array_static(iter);
            if (!iter) {
                matte_syntax_graph_print_compile_error(g, iter, "Object array literal missing end ']'");
                goto L_FAIL;
            }
            break;
          }

          case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {
            matteToken_t * start = iter;
            iter = iter->next;
            matteFunctionBlock_t * b = compile_function_block(g, block, functions, &iter);
            if (!b) {
                goto L_FAIL;
            }
            // to reference this new function as needed, the token for the 
            // function constructor will have its text replaced with a pointer
            matte_token_new_data(start, b, MATTE_TOKEN_DATA_TYPE__FUNCTION_BLOCK);    

            
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


          case MATTE_TOKEN_FUNCTION_ARG_BEGIN:
            iter = ff_skip_inner_arg(iter);
            if (!iter) {
                matte_syntax_graph_print_compile_error(g, iter, "Expression function call missing end ')'");
                goto L_FAIL;
            }
            break;
          case MATTE_TOKEN_OBJECT_ACCESSOR_BRACKET_START:
            iter = ff_skip_inner_bracket(iter);
            if (!iter) {
                matte_syntax_graph_print_compile_error(g, iter, "Barcket accessor missing end ']'");
                goto L_FAIL;
            }
            break;
                      
          case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: {
            matteToken_t * start = iter; 
            iter = iter->next; // skip (
            matteArray_t * inst = compile_expression(g, block, functions, &iter);
            if (!inst) {
                goto L_FAIL;
            }
            matteToken_t * end = iter->next; // skip )
            
            // to reference this expressions value, the string is REPLACED 
            // with a pointer to the array.
            
            matte_token_new_data(start, inst, MATTE_TOKEN_DATA_TYPE__ARRAY_INST);    
            

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

          default:;
            // assignment operators follow the same "sneaky" pattern,
            // where the post expression is always bound to the assignment 
            // operator token through the text pointer.
            if (token_is_assignment_derived(iter->ttype)) {
                matteToken_t * start = iter; 
                iter = iter->next; // skip =
                matteArray_t * inst = compile_expression(g, block, functions, &iter);
                if (!inst) {
                    goto L_FAIL;
                }
                matteToken_t * end = iter;
                
                // to reference this expressions value, the string is REPLACED 
                // with a pointer to the array.                
                matte_token_new_data(start, inst, MATTE_TOKEN_DATA_TYPE__ARRAY_INST);    
                

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
            iter = iter->next;

        }
    }
    
    iter = *src;



    // the idea is that an expression is a series of compute nodes
    // whose order of computation depends on its preceding operator
    appearanceID = 0;
    lvalue;

    // if an expression uses && or ||, we need special 
    // editing to do short circuiting 
    hasandor = 0;
    prev = NULL;
    while(iter->ttype != MATTE_TOKEN_MARKER_EXPRESSION_END) {
        int preOp = -1;
        int postOp = -1;
        int line = -1;
        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR1) {
            // operator first
            preOp = string_to_operator((matteString_t*)iter->data, iter->ttype);
            iter = iter->next;
        }
        line = iter->line;

        // by this point, all complex sub-expressions would have been 
        // reduced, so now we can just work with raw values
        matteArray_t * valueInst = compile_value(g, block, functions, &iter, &lvalue);
        if (!valueInst) {
            goto L_FAIL;
        }
        if (preOp != -1) {
            write_instruction__opr(valueInst, line - block->startingLine, preOp);
        }
        
        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR2) {
            // operator first
            postOp = string_to_operator((matteString_t*)iter->data, iter->ttype);
            iter = iter->next;
            
            
        // for assignment-derived tokens, we want to reduce 
        // the pattern 'lvalue [assignment] expression' into 
        // on value that can be worked with on a normal basis.
        //
        // The reason assignment is special is because we dont actually know 
        // its assignment (and what kind of assignment) until the lvalue is 
        // established.
        //
        // when combining, the assignment operator already has 
        // its operations packed within the assignment token.
        // These are unpacked when compiling the token as a value,
        // so we will do an extra compile and push it into the current node 
        } else if (token_is_assignment_derived(iter->ttype)) {
            if (!lvalue) {
                matte_syntax_graph_print_compile_error(g, iter, "Assignment operator in expression requires writable value on left-hand side of the assignment.");
                goto L_FAIL;
            }
            uint32_t size = matte_array_get_size(valueInst);
            matteBytecodeStubInstruction_t undo = matte_array_at(valueInst, matteBytecodeStubInstruction_t, size-1);

            int isSimpleReferrable = 1;
            uint32_t n;
            for(n = 0; n < size; ++n) {
                matteBytecodeStubInstruction_t u = matte_array_at(valueInst, matteBytecodeStubInstruction_t, n);
                if (u.info.opcode == MATTE_OPCODE_OLK) {
                    isSimpleReferrable = 0;
                    break;
                }
            }

            // Heres the fun part: lvalues are either
            // values that got reduced to a referrable OR an expression dot access OR a table lookup result.
            if (isSimpleReferrable) {
                //postOp = POST_OP_SYMBOLIC__ASSIGN_REFERRABLE + assignment_token_to_op_index(iter->ttype);        
                if (undo.info.opcode != MATTE_OPCODE_PRF) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing referrable token. (internal error)");
                    goto L_FAIL;
                }
                
                if (is_referrable_const(block, (uint32_t)(undo.data))) {
                    matte_syntax_graph_print_compile_error(g, iter, "Cannot assign new value to constant.");
                    goto L_FAIL;                    
                }
                // removed the referrable value, since thats already wrapped in the ARF
                matte_array_set_size(valueInst, size-1);
                write_instruction__arf(valueInst, line - block->startingLine, (uint32_t)(undo.data), assignment_token_to_op_index(iter->ttype));
                
            } else {
                // for handling assignment for the dot access and the [] lookup, 
                // the OLK instruction will be removed. This leaves both the 
                // object AND the key on the stack (since OLK would normally consume both)
                //postOp = POST_OP_SYMBOLIC__ASSIGN_MEMBER + assignment_token_to_op_index(iter->ttype) + (*((uint32_t*)undo.data) ? MATTE_OPERATOR_STATE_BRACKET : 0);
                matte_array_set_size(valueInst, size-1);
                if (undo.info.opcode != MATTE_OPCODE_OLK) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing lookup token. (internal error)");
                    goto L_FAIL;
                }
                write_instruction__osn(valueInst, line - block->startingLine, assignment_token_to_op_index(iter->ttype) + (((uint32_t)undo.data) ? MATTE_OPERATOR_STATE_BRACKET : 0));                

            }
            
            // basically wraps up the post-assignment expression
            matteArray_t * assignExp = compile_value(g, block, functions, &iter, &lvalue);
            if (!assignExp) {
                goto L_FAIL;
            }

            
            merge_instructions(assignExp, valueInst);
            valueInst = assignExp;

        }
        matteExpressionNode_t * exp = new_expression_node(
            preOp,
            postOp,
            appearanceID,
            line,
            valueInst
        );

        // node linking
        if (prev) prev->next = exp;
        prev = exp;

        matte_array_push(nodes, exp);
        appearanceID++;
    }


    // re-order based on precedence
    expression_node_sort(nodes);

    len = matte_array_get_size(nodes);
    for(i = 0; i < len; ++i) {
        matteExpressionNode_t * n = matte_array_at(nodes, matteExpressionNode_t *, i);

        // one operand always is applied before the 2op (post op)
        if (n->preOp > -1) {
        }

        // for 2-operand instructions, the first node is merged with the second node by 
        // putting instructions to compute it in order. 
        if (n->postOp > -1) {
            /*
            if (n->postOp >= POST_OP_SYMBOLIC__ASSIGN_MEMBER) {
                merge_instructions(n->next->value, n->value);
                write_instruction__osn(n->next->value, n->next->line, n->postOp - POST_OP_SYMBOLIC__ASSIGN_MEMBER);                
            } else if(n->postOp >= POST_OP_SYMBOLIC__ASSIGN_REFERRABLE) {
                uint32_t instSize = matte_array_get_size(n->value);
                matteBytecodeStubInstruction_t lastPRF = matte_array_at(n->value, matteBytecodeStubInstruction_t, instSize-1);
                matte_array_set_size(n->value, instSize-1);
                uint32_t referrable;
                memcpy(&referrable, &lastPRF.data[0], sizeof(uint32_t));

                if (lastPRF.info.opcode != MATTE_OPCODE_PRF) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing referrable token. (internal error)");
                    goto L_FAIL;
                }
                merge_instructions(n->next->value, n->value);
                write_instruction__arf(n->next->value, n->next->line, referrable, n->postOp - POST_OP_SYMBOLIC__ASSIGN_REFERRABLE);
            } else 
            */
            { // normal case.
            
                if (n->postOp == MATTE_OPERATOR_AND) {
                    // insert code to short circuit
                    //
                    // if n->value (the current node on the left) evaluates to 
                    // false, repush that false and jump to the end of THIS expression.
                    // else, keep going as if the branch didnt exist
                    //
                    // Since we don't know the end of the expression yet, we will add a special marker
                    // instead and replace it after the expression has ended.
                    matteBytecodeStubInstruction_t marker;
                    marker.info.lineOffset = n->line - block->startingLine;
                    marker.info.opcode = 0xff;
                    marker.data = 1;
                    matte_array_push(n->value, marker);
                    hasandor = 1;
                } else if (n->postOp == MATTE_OPERATOR_OR) {
                    matteBytecodeStubInstruction_t marker;
                    marker.info.lineOffset = n->line - block->startingLine;
                    marker.info.opcode = 0xff;
                    marker.data = 2;
                    matte_array_push(n->value, marker);                    
                    hasandor = 1;
                }
                // [1] op [2] -> [1 2 op]
                merge_instructions_rev(n->next->value, n->value);
                write_instruction__opr(n->next->value, n->next->line - block->startingLine, n->postOp);
            }


            // Remove all trances of n. Sorry!
            for(si = i+1; si < len; ++si) {
                matteExpressionNode_t * sn = matte_array_at(nodes, matteExpressionNode_t *, si);
                if (sn->next == n) sn->next = n->next; 
            }
        }
    }

    if (len) {
        assert(matte_array_at(nodes, matteExpressionNode_t *, len-1)->postOp == -1);
        merge_instructions(outInst, matte_array_at(nodes, matteExpressionNode_t *, len-1)->value);
    } else {
        // can be the case if the only expression nodes are assignment
    }


    // and/or uses placeholders because we need the short circuiting behavior
    // which requires knowing how large the expression is in terms of 
    // instructions.
    //
    // TODO: this currently does not add any logic to remove 
    // pre-pushed values on the stack for operations that would have been 
    // computed if not short-circuiting.
    if (hasandor) {
        uint32_t len = matte_array_get_size(outInst);
        for(i = 0; i < len; ++i) {
            matteBytecodeStubInstruction_t * inst = &matte_array_at(outInst, matteBytecodeStubInstruction_t, i);
            if (inst->info.opcode == 0xff) {
                switch((int)inst->data) {
                  case 1: {// AND 
                    inst->info.opcode = MATTE_OPCODE_SCA;
                    uint32_t skipAmt = len - i - 1;
                    inst->data = skipAmt;                    
                    break;
                  }
                  case 2: {// OLD
                    inst->info.opcode = MATTE_OPCODE_SCO;
                    uint32_t skipAmt = len - i - 1;
                    inst->data = skipAmt;  
                    break;
                  }

                }
            }
        }
    }


    // whew... now cleanup thanks
    // at this point, all nodes "value" attributes have been cleaned and transfered.
    for(i = 0; i < len; ++i) {
        matte_deallocate(matte_array_at(nodes, matteExpressionNode_t *, i));
    }
    matte_array_destroy(nodes);


    *src = iter->next; // skip expression marker!
    return outInst;
    
  L_FAIL:
    matte_array_destroy(nodes);
    matte_array_destroy(outInst);
    return NULL;
}





static int compile_statement(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * block,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    if (iter->ttype == MATTE_TOKEN_STATEMENT_END) {
        *src = iter->next;
        return 1;
    }
    if (!iter) {
        *src = NULL;
        return 0;
    }
    int varConst = 0;
    uint32_t oln = iter->line - block->startingLine;
    switch(iter->ttype) {
      // return statement
      case MATTE_TOKEN_RETURN: {
        iter = iter->next; // skip "return"
        matteArray_t * inst;
        if (!(inst = compile_expression(g, block, functions, &iter))) {
            // error reporting handled by compile_expression
            return -1;
        }
        // empty return
        if (matte_array_get_size(inst) == 0) {
            write_instruction__nem(block->instructions, oln);
            write_instruction__ret(block->instructions, oln);
        } else {
            // when computed, should leave one value on the stack
            merge_instructions(block->instructions, inst);
            write_instruction__ret(block->instructions, oln);
        }
        iter = iter->next; // skip ;
        break;
      }
      case MATTE_TOKEN_WHEN: {
        iter = iter->next; // skip when 
        iter = iter->next; // skip (
        matteArray_t * cond = compile_expression(g, block, functions, &iter);
        if (!cond) {
            return -1;
        }
        iter = iter->next; // skip )
        matteArray_t * ret;
        if (!(ret = compile_expression(g, block, functions, &iter))) {
            // error reporting handled by compile_expression
            return -1;            
        }
        write_instruction__ret(ret, oln);
        
        merge_instructions(block->instructions, cond);
        write_instruction__skp_insert(block->instructions, oln, matte_array_get_size(ret));
        merge_instructions(block->instructions, ret);    

        iter = iter->next; // skip ;
        break;
      }
      case MATTE_TOKEN_FOR: {
        iter = iter->next; // skip for
        iter = iter->next; // skip (
        matteArray_t * from = compile_expression(g, block, functions, &iter);
        if (!from) {
            return -1;
        }
        iter = iter->next; // skip ,
        matteArray_t * to = compile_expression(g, block, functions, &iter);
        if (!to) {
            return -1;
        }
        iter = iter->next; // skip )
        matteArray_t * func = compile_expression(g, block, functions, &iter);
        if (!func) {
            return -1;
        }


        
        merge_instructions(block->instructions, from);
        merge_instructions(block->instructions, to);
        merge_instructions(block->instructions, func);
        write_instruction__lop(block->instructions, oln);

        iter = iter->next; // skip ;
        break;
      }   


      case MATTE_TOKEN_FOREVER: {
        iter = iter->next; // skip forever 
        matteArray_t * func = compile_expression(g, block, functions, &iter);
        if (!func) {
            return -1;
        }

        merge_instructions(block->instructions, func);
        write_instruction__fvr(block->instructions, oln);

        iter = iter->next; // skip ;
        break;
      }

      case MATTE_TOKEN_FOREACH: {
        iter = iter->next; // skip foreach
        iter = iter->next; // skip (
        matteArray_t * from = compile_expression(g, block, functions, &iter);
        if (!from) {
            return -1;
        }
        iter = iter->next; // skip )
        matteArray_t * func = compile_expression(g, block, functions, &iter);
        if (!func) {
            return -1;
        }


        
        merge_instructions(block->instructions, from);
        merge_instructions(block->instructions, func);
        write_instruction__fch(block->instructions, oln);

        iter = iter->next; // skip ;
        break;
      }  


      
      case MATTE_TOKEN_DECLARE_CONST:
        varConst = 1;
      case MATTE_TOKEN_DECLARE: {
        if (iter->next->ttype == MATTE_TOKEN_VARIABLE_NAME && !strcmp("$", matte_string_get_c_str((matteString_t*)iter->next->data))) {
            matte_syntax_graph_print_compile_error(g, iter, "Cannot declare variable with special binding name $.");
            return -1;
        }
        if (iter->next->ttype == MATTE_TOKEN_VARIABLE_NAME && !strcmp("_", matte_string_get_c_str((matteString_t*)iter->next->data))) {
            matte_syntax_graph_print_compile_error(g, iter, "Cannot declare variable with special interface private accessor _.");
            return -1;
        }


        iter = iter->next; // declaration

        uint32_t gl = get_local_referrable(g, block, iter);
        if (gl != 0xffffffff) {
            iter = iter->next;
        } else {
            matteString_t * m = matte_string_create_from_c_str("Could not find local referrable of the given name '%s'", matte_string_get_c_str((matteString_t*)iter->data));
            matte_syntax_graph_print_compile_error(g, iter, matte_string_get_c_str(m));
            matte_string_destroy(m);
            return -1;
        }
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
        merge_instructions(block->instructions, inst);
        write_instruction__arf(block->instructions, oln, gl, 0); // no special operator


        break;
      }

      default: {
        // when computed, should leave one value on the stack
        matteArray_t * inst;
        if (!(inst = compile_expression(g, block, functions, &iter))) {
            return -1;
        }
        merge_instructions(block->instructions, inst);
        write_instruction__pop(block->instructions, oln, 1);
        iter = iter->next; // skip ;
      }
    }




    if (!iter) {
        *src = NULL;
        return 0;
    }

    while(iter && iter->ttype == MATTE_TOKEN_STATEMENT_END) {
        iter = iter->next;
    }
    *src = iter;
    return 1;
}



static matteFunctionBlock_t * compile_function_block(
    matteSyntaxGraphWalker_t * g, 
    matteFunctionBlock_t * parent,
    matteArray_t * functions, 
    matteToken_t ** src
) {
    matteToken_t * iter = *src;
    matteFunctionBlock_t * b = (matteFunctionBlock_t*)matte_allocate(sizeof(matteFunctionBlock_t));
    b->startingLine = iter->line;
    b->args = matte_array_create(sizeof(matteString_t *));
    b->strings = matte_array_create(sizeof(matteString_t *));
    b->locals = matte_array_create(sizeof(matteString_t *));
    b->local_isConst = matte_array_create(sizeof(int));
    b->instructions = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    b->captures = matte_array_create(sizeof(matteBytecodeStubCapture_t));
    b->captureNames = matte_array_create(sizeof(matteString_t *));
    b->capture_isConst = matte_array_create(sizeof(int));
    b->stubID = g->functionStubID++;
    b->isEmpty = 1;
    b->parent = parent;
    matteToken_t * funcStart;
    

    #ifdef MATTE_DEBUG__COMPILER
        printf("COMPILING FUNCTION %d (line %d:%d)\n", b->stubID, iter->line, iter->character);
    #endif


    // gather all args first
    // a proper call to compile_function_block should start AT the '('
    // if present (err, after the function constructor more accurately)
    if (b->stubID != 0) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH) {
            // dash functions implicitly call the pushed fucntion immediately with 0 args given.
            b->isDashed = 1;
            iter = iter->next;
            if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH) {
                matte_syntax_graph_print_compile_error(g, iter, "Multiple dash '<=' tokens given when only 1 is expected.");
                goto L_FAIL;

            }
        }    
    
        if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_BEGIN) {
            // args must ALWAYS be variable names or the vararg token
            iter = iter->next;
            
            if (iter && iter->ttype == MATTE_TOKEN_VARARG) {
                b->isVarArg = 1;
                iter = iter->next;
                matteString_t * arg = matte_string_clone((matteString_t*)iter->data);
                matte_array_push(b->args, arg);
                iter = iter->next;
            } else {
                while(iter && iter->ttype == MATTE_TOKEN_VARIABLE_NAME) {
                    matteString_t * arg = matte_string_clone((matteString_t*)iter->data);
                    matte_array_push(b->args, arg);
                    #ifdef MATTE_DEBUG__COMPILER
                        printf("  - Argument %d: %s\n", matte_array_get_size(b->args), matte_string_get_c_str(arg));
                    #endif
                    iter = iter->next;
                    
                    //
                    // typestrict arguments
                    //
                    if (iter->ttype == MATTE_TOKEN_FUNCTION_TYPESPEC) { // =>
                        b->isEmpty = 0; // typespecifying implies behavior
                        iter = iter->next; // skip =>
                        // first argument with type specified. This means we need to insert 
                        // "any" types for the others.
                        if (!b->typestrict_types) {
                            b->typestrict_types = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
                            if (matte_array_get_size(b->args) != 1) {
                                uint32_t i;
                                uint32_t len = matte_array_get_size(b->args)-1;
                                for(i = 0; i < len; ++i) {
                                    write_instruction__pto(b->typestrict_types, GET_LINE_OFFSET(b), 7); // any;
                                }
                            }                            
                        }
                        matteArray_t * expInst = compile_expression(
                            g,
                            parent,
                            functions,
                            &iter
                        );    
                        if (!expInst) {
                            goto L_FAIL;
                        }                      
                        merge_instructions(b->typestrict_types, expInst);

                    } else if (b->typestrict_types) {
                        // because the typestrict_types exists, that means we have a 
                        // tyestrict function, so any not specified still need an "any" type 
                        // object.
                        write_instruction__pto(b->typestrict_types, GET_LINE_OFFSET(b), 7); 
                    }   
                    
                    
                    // default value binding
                    //
                    // It only computes the expression IF the value is empty.
                    if (iter->ttype == MATTE_TOKEN_GENERAL_SPECIFIER) {
                        iter = iter->next; // skip :

                        write_instruction__prf(
                            b->instructions, 
                            GET_LINE_OFFSET(b),                             
                            matte_array_get_size(b->args)
                        );

                        write_instruction__nem(
                            b->instructions, 
                            GET_LINE_OFFSET(b)
                        );
                        
                        write_instruction__opr(
                            b->instructions, 
                            GET_LINE_OFFSET(b),
                            MATTE_OPERATOR_EQ
                        );

                        matteArray_t * expInst = compile_expression(
                            g,
                            b,
                            functions,
                            &iter
                        );                         

                        write_instruction__skp_insert(
                            b->instructions, 
                            GET_LINE_OFFSET(b),
                            matte_array_get_size(expInst)+1 // skip expression+arf
                        );

                        write_instruction__arf(
                            expInst, 
                            GET_LINE_OFFSET(b),
                            matte_array_get_size(b->args), 
                            0
                        );
                        
                        merge_instructions(
                            b->instructions,
                            expInst
                        );
                    }
                    
                    if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_END) break;
                    
                    
                    if (iter->ttype != MATTE_TOKEN_FUNCTION_ARG_SEPARATOR) {
                        matte_syntax_graph_print_compile_error(g, iter, "Expected ',' separator for arguments");
                        goto L_FAIL;                
                    }
                    iter = iter->next;
                }
                
                if (matte_array_get_size(b->args) > FUNCTION_ARG_MAX)
                    matte_syntax_graph_print_compile_error(g, iter, "Function exceeds argument count limit.");
            }       

            if (iter->ttype != MATTE_TOKEN_FUNCTION_ARG_END) {
                matte_syntax_graph_print_compile_error(g, iter, "Expected ')' to end function arguments");
                goto L_FAIL;
            }
            iter = iter->next;
            
            
            //
            // Return value type 
            //
            if (iter->ttype == MATTE_TOKEN_FUNCTION_TYPESPEC) {
                b->isEmpty = 0; // typespecifying implies behavior
                iter = iter->next; // skip =>
                if (!b->typestrict_types) {
                    b->typestrict_types = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

                    uint32_t i;
                    uint32_t len = matte_array_get_size(b->args);
                    for(i = 0; i < len; ++i) {
                        write_instruction__pto(b->typestrict_types, GET_LINE_OFFSET(b), 7); // any;
                    }

                }
            
                matteArray_t * expInst = compile_expression(
                    g,
                    parent,
                    functions,
                    &iter
                );    
                if (!expInst) {
                    goto L_FAIL;
                }                      
                merge_instructions(b->typestrict_types, expInst);

            } else if (b->typestrict_types) { 
                b->isEmpty = 0; // type specifying implies behavior
                // no return value type, but we're in strict mode. So enter an "any" type.
                write_instruction__pto(b->typestrict_types, GET_LINE_OFFSET(b), 7);             
            }
        }
        if (b->typestrict_types) {
            b->typestrict = matte_array_get_size(b->args) + 1;
        }

        if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR_INLINE) {
            b->isEmpty = 0; // inlines cant be empty;
            iter = iter->next;
            matteArray_t * expInst = compile_expression(
                g,
                b,
                functions,
                &iter
            );    
            if (!expInst) {
                goto L_FAIL;
            }                      
            merge_instructions(b->instructions, expInst);
            *src = iter;
            return b;                                
        


        // most situations will require that the block begin '{' token 
        // exists already. This will be true EXCEPT in the toplevel function call (root stub)        
        } else if (iter->ttype != MATTE_TOKEN_FUNCTION_BEGIN) {
            matte_syntax_graph_print_compile_error(g, iter, "Missing function block begin brace. '{'");
            goto L_FAIL;
        }
        iter = iter->next;
    }

    if (parent == NULL) { // toplevel script always contains one argument: parameters
        matteString_t * arg = matte_string_create_from_c_str("parameters");
        matte_array_push(b->args, arg);
    }
    // next find all locals and static strings
    funcStart = iter;
    while(iter && iter->ttype != MATTE_TOKEN_FUNCTION_END) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR ||  
            iter->ttype == MATTE_TOKEN_FUNCTION_LISTEN) {
            ff_skip_inner_function(&iter);
        } else if (iter->ttype == MATTE_TOKEN_DECLARE ||
            iter->ttype == MATTE_TOKEN_DECLARE_CONST) {
            int isconst = iter->ttype == MATTE_TOKEN_DECLARE_CONST;

            iter = iter->next;
            if (iter->ttype != MATTE_TOKEN_VARIABLE_NAME) {
                matte_syntax_graph_print_compile_error(g, iter, "Expected local variable name.");
                goto L_FAIL;
            }
            
            matteString_t * local = matte_string_clone((matteString_t*)iter->data);
            matte_array_push(b->locals, local);
            matte_array_push(b->local_isConst, isconst);
            
            #ifdef MATTE_DEBUG__COMPILER
                printf("  - Local%s %d: %s\n", isconst? "(const)" : "", matte_array_get_size(b->locals), matte_string_get_c_str(local));
            #endif
        } 
        iter = iter->next;
    }
    
    if (matte_array_get_size(b->locals) > FUNCTION_LOCAL_MAX)
        matte_syntax_graph_print_compile_error(g, iter, "Function exceeds local variable count limit.");

    // reset
    iter = funcStart;



    // now that we have all the locals and args, we can start emitting bytecode.
    if (iter && iter->ttype != MATTE_TOKEN_FUNCTION_END) {
        b->isEmpty = 0;
        int status;
        do {
            status = compile_statement(g, b, functions, &iter);
            if (!(iter && iter->ttype != MATTE_TOKEN_FUNCTION_END)) break;
        } while(status == 1);

        if (status == -1) {
            // should be clean
            goto L_FAIL;
        }
    }
    
    // in the case that a user has not explicitly placed a return, then 
    // we by default "return empty"
    // we aren't C after all...
    if (!b->isEmpty && iter) {
        write_instruction__nem(b->instructions, GET_LINE_OFFSET(b));
        write_instruction__ret(b->instructions, GET_LINE_OFFSET(b));
    }
    
    // for every except stubID == 0, there should be an end brace here. Consume it.
    if (iter && iter->ttype == MATTE_TOKEN_FUNCTION_END) {
        *src = iter->next;
    } else {
        *src = iter;
    }

    return b;
  L_FAIL:
    function_block_destroy(b);
    return NULL;
}

matteArray_t * matte_syntax_graph_compile(matteSyntaxGraphWalker_t * g) {
    matteToken_t * iter = g->first;
    matteArray_t * arr = matte_array_create(sizeof(matteFunctionBlock_t *));
    matteFunctionBlock_t * root;
    if (!(root = compile_function_block(g, NULL, arr, &iter))) {
        return NULL;        
    }
    if (g->first) {
        write_instruction__nem(root->instructions, g->first->line);
        write_instruction__ret(root->instructions, g->first->line);
    }
    matte_array_push(arr, root);
    return arr;
}



#define WRITE_BYTES(__T__, __VAL__) matte_array_push_n(byteout, &(__VAL__), sizeof(__T__));
#define WRITE_NBYTES(__N__, __VALP__) matte_array_push_n(byteout, (__VALP__), __N__);

static void write_unistring(matteArray_t * byteout, matteString_t * str) {
    uint32_t len = matte_string_get_utf8_length(str);
    WRITE_BYTES(uint32_t, len);
    WRITE_NBYTES(len, matte_string_get_utf8_data(str));
}

void * matte_function_block_array_to_bytecode(
    matteArray_t * arr, 
    uint32_t * size
) {
    *size = 0;
    matteArray_t * byteout = matte_array_create(sizeof(uint8_t));
    uint32_t i;
    uint32_t len = matte_array_get_size(arr);
    uint32_t n;
    uint8_t nSlots;
    uint16_t nCaps;
    uint32_t nInst;
    uint32_t nStrings;

    uint8_t tag[] = {
        'M', 'A', 'T', 0x01, 0x06, 'B', 0x1
    };
    for(i = 0; i < len; ++i) {
        matteFunctionBlock_t * block = matte_array_at(arr, matteFunctionBlock_t *, i);

        nSlots = 1;
        WRITE_NBYTES(7, tag); // HEADER + bytecode version
        WRITE_BYTES(uint32_t, block->stubID);
        nSlots = block->isVarArg;
        WRITE_BYTES(uint8_t, nSlots);

        nSlots = matte_array_get_size(block->args);
        WRITE_BYTES(uint8_t, nSlots);
        for(n = 0; n < nSlots; ++n) {
            write_unistring(byteout, matte_array_at(block->args, matteString_t *, n));
        }

        nSlots = matte_array_get_size(block->locals);
        WRITE_BYTES(uint8_t, nSlots);
        for(n = 0; n < nSlots; ++n) {
            write_unistring(byteout, matte_array_at(block->locals, matteString_t *, n));
        }

        nStrings = matte_array_get_size(block->strings);
        WRITE_BYTES(uint32_t, nStrings);
        for(n = 0; n < nStrings; ++n) {
            write_unistring(byteout, matte_array_at(block->strings, matteString_t *, n));
        }


        nCaps = matte_array_get_size(block->captures);
        WRITE_BYTES(uint16_t, nCaps);
        WRITE_NBYTES(nCaps * (sizeof(uint32_t) + sizeof(uint32_t)), matte_array_get_data(block->captures));

        nInst = matte_array_get_size(block->instructions);
        WRITE_BYTES(uint32_t, nInst);

        WRITE_BYTES(uint32_t, block->startingLine);

        for(n = 0; n < nInst; ++n) {
            matteBytecodeStubInstruction_t * inst = &matte_array_at(block->instructions, matteBytecodeStubInstruction_t, n);
            WRITE_BYTES(uint16_t, inst->info.lineOffset);
            WRITE_BYTES(uint8_t, inst->info.opcode);
            WRITE_BYTES(double, inst->data);        
        }

        function_block_destroy(block);
    }


    *size = matte_array_get_size(byteout);
    uint8_t * out = (uint8_t*)matte_allocate(*size);
    memcpy(out, matte_array_get_data(byteout), *size);

    matte_array_destroy(byteout);
    matte_array_destroy(arr);
    return out;
}
