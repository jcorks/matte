#include "matte_compiler.h"
#include "matte_array.h"
#include "matte_string.h"
#include "matte_table.h"
#include "matte_bytecode_stub.h"
#include "matte_opcode.h"
#include "matte_compiler__syntax_graph.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>






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
    matteTokenizer_t *,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch, void * userdata), 
    void * userdata
);

static void matte_syntax_graph_walker_destroy(matteSyntaxGraphWalker_t *);

// Returns the first parsed token from the user source
static matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraphWalker_t * g);

// compiles all nodes into bytecode function blocks.
typedef struct matteFunctionBlock_t matteFunctionBlock_t;
struct matteFunctionBlock_t {
    uint32_t stubID;
    // count of typestrict types. args + 1
    uint32_t typestrict;

    // whether the function is run immediately before parsing of 
    // additional tokens.
    int isDashed;

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
    const uint8_t * source, 
    uint32_t len,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void * userdata),
    void * userdata
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraphWalker_t * st = matte_syntax_graph_walker_create(w, onError, userdata);


   
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
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    matteTokenizer_t * w = matte_tokenizer_create(source, len);
    matteSyntaxGraphWalker_t * st = matte_syntax_graph_walker_create(w, onError, userdata);


   
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

    void * bytecode = matte_function_block_array_to_bytecode(arr, size);
    matte_syntax_graph_walker_destroy(st);


    // cleanup :(
    // especially those gosh darn function blocks
    return bytecode;
}

uint8_t * matte_compiler_run_with_named_references(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    OPTION__NAMED_REFERENCES = 1;
    return matte_compiler_run_base(
        source, len, size, onError, userdata
    );
}

uint8_t * matte_compiler_run(
    const uint8_t * source, 
    uint32_t len,
    uint32_t * size,
    void(*onError)(const matteString_t * s, uint32_t line, uint32_t ch, void *),
    void * userdata
) {
    OPTION__NAMED_REFERENCES = 0;
    return matte_compiler_run_base(
        source, len, size, onError, userdata
    );
}




/////////////////////////////////////////////////////////////

// walker;

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
                (c == '_') || // underscore
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

#define MAX_ASCII_BUFLEN 256

struct matteTokenizer_t {
    uint8_t * backup;
    uint8_t * iter;
    uint8_t * source;
    uint32_t line;
    uint32_t character;

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
    if (t->ttype == MATTE_TOKEN_EXPRESSION_GROUP_BEGIN ||
        t->ttype == MATTE_TOKEN_EXTERNAL_GATE ||
        t->ttype == MATTE_TOKEN_EXTERNAL_MATCH) {
        matte_array_destroy((matteArray_t*)t->text);
    } else if (t->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {
        // is a pointer to a function block, so dont touch.
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

static matteToken_t * matte_tokenizer_consume_exact(
    matteTokenizer_t * t,
    uint32_t line,
    uint32_t ch,
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
    tokenizer_strip(t);
    uint32_t currentLine = t->line;
    uint32_t currentCh = t->character;

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
        if (sscanf(matte_string_get_c_str(out), "%lf", &f) == 1) {
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
        int single = 0;
        switch(c) {
          case '\'':
            single = 1;
          case '"':
            t->character++;
            break;
          default:
            t->iter = t->backup;
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
                return NULL;            
                
              case '\\': // escape character 
                switch(utf8_next_char(&t->iter)) {
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
                    t->iter = t->backup;
                    matte_string_destroy(text);
                    return NULL;            
                  
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

      case MATTE_TOKEN_EXTERNAL_NOOP: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "noop");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_GATE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "if");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_LOOP: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "loop");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_FOR: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "for");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_FOREACH: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "foreach");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_MATCH: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "match");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "getExternalFunction");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "import");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_REMOVE_KEY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "removeKey");
        break;
      }

      case MATTE_TOKEN_EXTERNAL_SET_ATTRIBUTES: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "setAttributes");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_GET_ATTRIBUTES: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "getAttributes");
        break;
      }


      case MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Boolean");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEEMPTY: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Empty");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPENUMBER: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Number");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPESTRING: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "String");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEOBJECT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Object");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPETYPE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Type");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPEFUNCTION: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "Function");
        break;
      }


      case MATTE_TOKEN_EXTERNAL_INTROSPECT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "introspect");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_TYPE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "newtype");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_INSTANTIATE: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "instantiate");
        break;
      }

      case MATTE_TOKEN_EXTERNAL_PRINT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "print");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_ERROR: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "error");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_SEND: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "send");
        break;
      }
      case MATTE_TOKEN_EXTERNAL_LISTEN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "listen");
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

      case MATTE_TOKEN_ASSIGNMENT_ADD: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "+=");
        break;
      }
      case MATTE_TOKEN_ASSIGNMENT_SUB: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "-=");
        break;
      }
      case MATTE_TOKEN_ASSIGNMENT_MULT:  {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "*=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_DIV: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "/=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_MOD: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "%=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_POW: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "**=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_AND:  {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "&=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_OR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "|=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_XOR: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "^=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_BLEFT: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "<<=");
        break;
      }

      case MATTE_TOKEN_ASSIGNMENT_BRIGHT: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, ">>=");
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
            t->character++;
            t->backup = t->iter;
            c = utf8_next_char(&t->iter);
            switch(c) {
              case '>':
                t->character++;
                t->backup = t->iter;
                return new_token(
                    matte_string_create_from_c_str("-%c", c),
                    currentLine,
                    currentCh,
                    ty
                );            
                break;

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
            return NULL;
        }

        break;
      }
      case MATTE_TOKEN_OBJECT_LITERAL_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '{');
        break;          
      }
      case MATTE_TOKEN_OBJECT_LITERAL_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '}');
        break;
      }

      case MATTE_TOKEN_OBJECT_DEF_PROP: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ':');
        break;          
      }

      case MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR: {
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
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "<@>");
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
      case MATTE_TOKEN_IMPLICATION_START: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '(');
        break;
      }
      case MATTE_TOKEN_IMPLICATION_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ')');
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
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "::");
        break;  
      }
      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR_DASH: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "<=");
        break;  
      }

      case MATTE_TOKEN_FUNCTION_TYPESPEC: {
        return matte_tokenizer_consume_exact(t, currentLine, currentCh, ty, "=>");
        break;  
      }

      case MATTE_TOKEN_WHEN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "when");
        break;            
      }
      case MATTE_TOKEN_GATE_RETURN: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "else");
        break; 
      }
      case MATTE_TOKEN_MATCH_BEGIN: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '{');
        break; 
      }
      case MATTE_TOKEN_MATCH_END: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, '}');
        break; 
      }
      case MATTE_TOKEN_MATCH_IMPLIES: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ':');
        break; 
      }
      case MATTE_TOKEN_MATCH_SEPARATOR: {
        return matte_tokenizer_consume_char(t, currentLine, currentCh, ty, ',');
        break; 
      }
      case MATTE_TOKEN_MATCH_DEFAULT: {
        return matte_tokenizer_consume_word(t, currentLine, currentCh, ty, "default");
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
    assert(!"Should not be reached");
}

// Returns whether the tokenizer has reached the end of the inout text.
int matte_tokenizer_is_end(matteTokenizer_t * t){
    tokenizer_strip(t);
    return t->iter[0] == 0;
}


















//////////////////////////////////////////
// syntax graph 



struct matteSyntaxGraphWalker_t {
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

static matteSyntaxGraph_t * GRAPHSRC = NULL;

matteSyntaxGraphWalker_t * matte_syntax_graph_walker_create(
    matteTokenizer_t * t,
    void (*onError)(const matteString_t * errMessage, uint32_t line, uint32_t ch, void * userdata),
    void * userdata
) {
    matteSyntaxGraphWalker_t * out = calloc(1, sizeof(matteTokenizer_t));
    out->tokenizer = t;
    out->first = out->last = NULL;
    out->onError = onError;
    out->onErrorData = userdata;
    out->tried = matte_table_create_hash_pointer();

    if (!GRAPHSRC) {
        GRAPHSRC = matte_syntax_graph_create();
    }
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
    free(t);
}


static void matte_syntax_graph_print_error(
    matteSyntaxGraphWalker_t * graph,
    matteSyntaxGraphNode_t * node
) {
    matteString_t * message = matte_string_create_from_c_str("Syntax Error: Expected ");
    switch(node->type) {
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        uint32_t i;
        uint32_t len = node->token.count;
        if (len == 1) {
            matte_string_concat(message, matte_syntax_graph_get_token_name(GRAPHSRC, node->token.refs[0]));
        } else {
            for(i = 0; i < len; ++i) {
                if (i == len - 1) {
                    matte_string_concat_printf(message, ", ");
                } else if (i == 0) {

                } else {
                    matte_string_concat_printf(message, ", or ");
                }             
                matte_string_concat(message, matte_syntax_graph_get_token_name(GRAPHSRC, node->token.refs[i]));
            }
        }
        break;
      }
      case MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT: {
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(GRAPHSRC, node->construct);
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
        matte_tokenizer_current_character(graph->tokenizer),
        graph->onErrorData
    );
    matte_string_destroy(message);
}

#ifdef MATTE_DEBUG
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

#endif



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
    
    switch(node->type) {
      // node is simply a token / token set.
      // We try each token in order. If it parses successfully,
      // we continue. If not, we try the next token.
      // If we exhaust all tokens, we have failed.
      case MATTE_SYNTAX_GRAPH_NODE__TOKEN: {
        #ifdef MATTE_DEBUG__COMPILER
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__TOKEN: %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);
        #endif
        uint32_t i;
        uint32_t len = node->token.count;
        for(i = 0; i < len; ++i) {
            #ifdef MATTE_DEBUG__COMPILER
                printf("     - trying to parse token as %s...", matte_string_get_c_str(matte_syntax_graph_get_token_name(GRAPHSRC, node->token.refs[i])));
            #endif


            matteToken_t * newT;
            // markers do not consume text.
            if (node->token.marker) {
                newT = new_token(
                    matte_string_create_from_c_str(""),
                    graph->tokenizer->line,
                    graph->tokenizer->character,
                    node->token.refs[i]
                ); 

            // normal case: consume text needed based on token
            } else {
                newT = matte_tokenizer_next(graph->tokenizer, node->token.refs[i]);
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
            matte_syntax_graph_print_error(graph, node);
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
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__SPLIT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
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
            matte_syntax_graph_print_error(graph, node);
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
            printf("WALKING: @MATTE_SYNTAX_GRAPH_NODE__PARENT_REDIRECT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif

        uint32_t level = node->upLevel;
        matteSyntaxGraphNode_t * n = node;
        while(level--) {
            if (n->parent) n = n->parent;
        }
        if (!n) {
            if (!silent) {
                matte_syntax_graph_print_error(graph, node);
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
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__END %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
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
            printf("WALKING: MATTE_SYNTAX_GRAPH_NODE__CONSTRUCT %s (next c == '%c')\n", matte_string_get_c_str(matte_syntax_graph_get_root(GRAPHSRC, constructID)->name), matte_tokenizer_peek_next(graph->tokenizer));
            fflush(stdout);

        #endif
        matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(GRAPHSRC, node->construct);

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
                while(out = matte_syntax_graph_walk_helper(graph, out, node->construct, 0, error)) {
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
            matte_syntax_graph_print_error(graph, node);
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
    if (!matte_syntax_graph_is_construct(GRAPHSRC, constructID)) {
        matteString_t * str = matte_string_create_from_c_str("Internal error (no such constrctID)");
        graph->onError(str, 0, 0, graph->onErrorData);
        matte_string_destroy(str);
        return 0;
    }

    matteSyntaxGraphRoot_t * root = matte_syntax_graph_get_root(GRAPHSRC, constructID);
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


matteToken_t * matte_syntax_graph_get_first(matteSyntaxGraphWalker_t * g) {
    return g->first;
}



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
        matte_string_get_c_str(matte_syntax_graph_get_token_name(GRAPHSRC, t->ttype)),
        matte_string_get_c_str(t->text)
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
    free(t);

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

    // special: always refers to the calling context.
    if (!strcmp(matte_string_get_c_str(iter->text), "context")) {
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

    return 0xffffffff;
}

// returns whether the referrable for this block is constant.
// This will go through existing captures as well.
static int is_referrable_const(matteFunctionBlock_t * block, uint32_t referrable) {
    // The "context" referrable is implicitly constant.
    // be reasonable you sneaky ppl....
    if (referrable == 0) return 1;

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
    int found = 0;
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
        write_instruction__prf(inst, iter->line, referrable);
        *src = iter->next;
        return inst;
    } else {
        // first look through existing captures
        uint32_t i;
        uint32_t len;

        len = matte_array_get_size(block->captureNames);
        for(i = 0; i < len; ++i) {
            if (matte_string_test_eq(iter->text, matte_array_at(block->captureNames, matteString_t *, i))) {
                write_instruction__prf(inst, iter->line, i+1+matte_array_get_size(block->locals)+matte_array_get_size(block->args));
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
                if (matte_string_test_eq(iter->text, matte_array_at(block->args, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = block->stubID;
                    capture.referrable = i+1;
                    write_instruction__prf(
                        inst, iter->line, 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matte_array_push(blockSrc->captures, capture);
                    matteString_t * str = matte_string_clone(iter->text);
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
                if (matte_string_test_eq(iter->text, matte_array_at(block->locals, matteString_t *, i))) {
                    matteBytecodeStubCapture_t capture;
                    capture.stubID = block->stubID;
                    capture.referrable = i+offset;
                    write_instruction__prf(
                        inst, iter->line, 
                        1+matte_array_get_size(blockSrc->locals)+
                          matte_array_get_size(blockSrc->args) +
                          matte_array_get_size(blockSrc->captures)
                    );
                    matte_array_push(blockSrc->captures, capture);
                    matteString_t * str = matte_string_clone(iter->text);
                    matte_array_push(blockSrc->captureNames, str);
                    isconst = is_referrable_const(block, i + 1 + matte_array_get_size(block->args));
                    matte_array_push(blockSrc->capture_isConst, isconst);

                    *src = iter->next;
                    return inst;                    

                }
            }
            block = block->parent;
        }
    }

    if (OPTION__NAMED_REFERENCES) {
        uint32_t i = function_intern_string(blockOrig, iter->text);
        write_instruction__pnr(inst, iter->line, i);
        *src = iter->next;
        return inst;
    } else {
        matteString_t * m = matte_string_create_from_c_str("Undefined variable '%s'", matte_string_get_c_str(iter->text));
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
            case '=': return MATTE_OPERATOR_EQ; break;
            default:;
        }
        break;

        case '~': return MATTE_OPERATOR_BITWISE_NOT; break;
        break;

        case '#': return MATTE_OPERATOR_POUND; break;
        case '?': return MATTE_OPERATOR_TERNARY; break;
        case '$': return MATTE_OPERATOR_TOKEN; break;
        case '^': return MATTE_OPERATOR_CARET; break;
        case '%': return MATTE_OPERATOR_MODULO; break;


        break;
        default:;
    }
    return -1;
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
      case MATTE_OPERATOR_TOKEN: // $ 1 operand
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
    matteExpressionNode_t * out = calloc(1, sizeof(matteExpressionNode_t));
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
        write_instruction__nbl(inst, iter->line, !strcmp(matte_string_get_c_str(iter->text), "true"));
        *src = iter->next;
        return inst;

      case MATTE_TOKEN_LITERAL_NUMBER: {
        double val = 0.0;
        sscanf(matte_string_get_c_str(iter->text), "%lf", &val);
        write_instruction__nnm(inst, iter->line, val);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_LITERAL_EMPTY: {
        write_instruction__nem(inst, iter->line);
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_LITERAL_STRING: {
        matteString_t * str = iter->text;
        uint32_t strl = matte_string_get_length(str);
        if(strl == 2) {
            matteString_t * empty = matte_string_create();
            write_instruction__nst(inst, iter->line, function_intern_string(block, empty));
            matte_string_destroy(empty);
        } else {        
            write_instruction__nst(inst, iter->line, function_intern_string(block, matte_string_get_substr(iter->text, 1, strl-2)));
        }
        *src = iter->next;
        return inst;
      }


      case MATTE_TOKEN_EXTERNAL_PRINT: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_PRINT);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_SEND: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_SEND);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_LISTEN: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_LISTEN);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_ERROR: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_ERROR);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_FOREACH: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_FOREACH);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_GETEXTERNALFUNCTION: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_GETEXTERNALFUNCTION);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_LOOP: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_LOOP);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_FOR: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_FOR);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_IMPORT: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_IMPORT);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_REMOVE_KEY: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_REMOVE_KEY);
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_EXTERNAL_SET_ATTRIBUTES: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_SET_ATTRIBUTES);
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_EXTERNAL_GET_ATTRIBUTES: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_GET_ATTRIBUTES);
        *src = iter->next;
        return inst;
      }

      
      case MATTE_TOKEN_EXTERNAL_TYPEEMPTY: {
        write_instruction__pto(inst, iter->line, 0);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEBOOLEAN: {
        write_instruction__pto(inst, iter->line, 1);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPENUMBER: {
        write_instruction__pto(inst, iter->line, 2);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPESTRING: {
        write_instruction__pto(inst, iter->line, 3);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEOBJECT: {
        write_instruction__pto(inst, iter->line, 4);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPEFUNCTION: {
        write_instruction__pto(inst, iter->line, 5);
        *src = iter->next;
        return inst;
      } 
      case MATTE_TOKEN_EXTERNAL_TYPETYPE: {
        write_instruction__pto(inst, iter->line, 6);
        *src = iter->next;
        return inst;
      } 


           
      case MATTE_TOKEN_EXTERNAL_INTROSPECT: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_INTROSPECT);
        write_instruction__cal(inst, iter->line, 0);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_TYPE: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_TYPE);
        *src = iter->next;
        return inst;
      }
      case MATTE_TOKEN_EXTERNAL_INSTANTIATE: {
        write_instruction__ext(inst, iter->line, MATTE_EXT_CALL_INSTANTIATE);
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
            if (iter->ttype != MATTE_TOKEN_OBJECT_ARRAY_END) 
                iter = iter->next; // skip ,

            merge_instructions(inst, expInst);
            itemCount++;
        }
        write_instruction__nar(inst, iter->line, itemCount);
        *src = iter->next; // skip ]
        return inst;
        break;
      }



      // object literal
      case MATTE_TOKEN_OBJECT_LITERAL_BEGIN: {
        iter = iter->next;
        uint32_t nPairs = 0;
        while(iter->ttype != MATTE_TOKEN_OBJECT_LITERAL_END) {
            // key : value,
            // OR
            // key :: function

            // special case: variable name is treated like a string for convenience
            if (iter->ttype == MATTE_TOKEN_VARIABLE_NAME &&
                (iter->next->next->ttype == MATTE_TOKEN_OBJECT_DEF_PROP ||
                 iter->next->next->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR)
            ) {
                
                write_instruction__nst(inst, iter->line, function_intern_string(block, iter->text));
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

            if (iter->ttype == MATTE_TOKEN_OBJECT_DEF_PROP) {
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


            if (iter->ttype == MATTE_TOKEN_OBJECT_LITERAL_SEPARATOR)
                iter = iter->next;                
            nPairs++;
        }
        if (nPairs == 0)
            write_instruction__nob(inst, iter->line);
        else
            write_instruction__nso(inst, iter->line, nPairs);
        *src = iter->next; // skip } 
        return inst;
      }




      case MATTE_TOKEN_EXPRESSION_GROUP_BEGIN: { 
        matteArray_t * arr = (matteArray_t *)iter->text; // the sneaky in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }


      case MATTE_TOKEN_EXTERNAL_GATE: { 
        matteArray_t * arr = (matteArray_t *)iter->text; // the sneaky II in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_EXTERNAL_MATCH: { 
        matteArray_t * arr = (matteArray_t *)iter->text; // the sneaky III in action....
        merge_instructions(inst, matte_array_clone(arr));
        *src = iter->next;
        return inst;
      }

      case MATTE_TOKEN_FUNCTION_CONSTRUCTOR: {

        matteFunctionBlock_t * fn = (matteFunctionBlock_t *)iter->text; // the sneaky IV in action....
        if (fn->typestrict) {
            merge_instructions(inst, matte_array_clone(fn->typestrict_types));
            write_instruction__sfs(
                inst, 
                iter->line,
                fn->typestrict
            );
            write_instruction__nfn(
                inst, 
                iter->line,
                fn->stubID
            );
        
        } else {
            write_instruction__nfn(
                inst, 
                iter->line,
                fn->stubID
            );
        }
        if (fn->isDashed) {
            write_instruction__cal(
                inst, 
                iter->line,
                0
            );        
        }
        *src = iter->next;
        return inst;
      }
    }

    matte_syntax_graph_print_compile_error(g, iter, "Unrecognized value token (internal error)");
    matte_array_destroy(inst);
    return NULL;               

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
                write_instruction__nst(inst, iter->line, function_intern_string(block, iter->text));
                write_instruction__olk(inst, iter->line, 0);
                *lvalue = 1;
                iter = iter->next;
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
            write_instruction__olk(inst, iter->line, 1);
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
        } //else if (square brackets accessor)

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
    while(iter->ttype != MATTE_TOKEN_FUNCTION_ARG_END) {
        matteArray_t * exp = compile_expression(g, block, functions, &iter);
        if (!exp) {
            goto L_FAIL;
        }
        merge_instructions(inst, exp); // push argument
        argCount++;
        if (iter->ttype == MATTE_TOKEN_FUNCTION_ARG_SEPARATOR)
            iter = iter->next; // skip ,
    }

    
    write_instruction__cal(inst, iter->line, argCount);
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
    uint32_t pivotDistance = 0;
    for(i = 0; i < matte_array_get_size(conditionExpressions); ++i) {
        pivotDistance += 4+matte_array_get_size(matte_array_at(conditionExpressions, matchcondition, i).condition);
    }

    uint32_t endDistance = 0;
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
        write_instruction__cpy(instOut, c.line);
        merge_instructions(instOut, c.condition);
        write_instruction__opr(instOut, c.line, MATTE_OPERATOR_EQ);
        write_instruction__skp_insert(instOut, c.line, 1);
        pivotDistance -= 4+len;
        write_instruction__asp(instOut, c.line, pivotDistance+r.offset);
    }
    assert(pivotDistance == 0);
    matte_array_destroy(conditionExpressions);

    write_instruction__pop(instOut, defaultLine, 1); endDistance--;
    if (defaultExpression) {
        endDistance-=matte_array_get_size(defaultExpression);
        merge_instructions(instOut, defaultExpression);
    } else {
        write_instruction__nem(instOut, defaultLine);endDistance--;
    }
    write_instruction__asp(instOut, defaultLine, endDistance-1);endDistance--;


    for(i = 0; i < matte_array_get_size(resultExpressions); ++i) {
        matchresult r = matte_array_at(resultExpressions, matchresult, i);
        uint32_t len = matte_array_get_size(r.result);
        write_instruction__pop(instOut, defaultLine, 1);
        merge_instructions(instOut, r.result);
        endDistance -= len + 2;
        write_instruction__asp(instOut, defaultLine, endDistance);
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
    }
    return 0;
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
            matte_string_destroy(start->text);   // VERY SNEAKY III
            start->text = (matteString_t *)inst; // VERY SNEAKIER III
            

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
            if (!inst) {
                goto L_FAIL;
            }


            // only true, else empty
            if (iter->ttype == MATTE_TOKEN_GATE_RETURN)  {
                iter = iter->next; // skip ":"

                iffalse = compile_expression(g, block, functions, &iter);
                if (!inst) {
                    goto L_FAIL;
                }
            } else {
                iffalse = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
                write_instruction__nem(iffalse, start->line);
            }




            matteToken_t * end = iter;
            write_instruction__skp_insert(inst, start->line, matte_array_get_size(iftrue)+1); // +1 for the ASP
            merge_instructions(inst, iftrue);
            write_instruction__asp(inst, start->line, matte_array_get_size(iffalse));
            merge_instructions(inst, iffalse);

            
            // to reference this expressions value, the string is REPLACED 
            // with a pointer to the array.
            
            matte_string_destroy(start->text);   // VERY SNEAKY II
            start->text = (matteString_t *)inst; // VERY SNEAKIER II
            

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
            matte_string_destroy(start->text);   // VERY SNEAKY IV
            start->text = (matteString_t *)b; // VERY SNEAKIER IV

            
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

          default:;
            iter = iter->next;

        }
    }
    
    iter = *src;



    // the idea is that an expression is a series of compute nodes
    // whose order of computation depends on its preceding operator
    int appearanceID = 0;
    int lvalue;
    int vstartType;

    // if an expression uses && or ||, we need special 
    // editing to do short circuiting 
    int hasandor = 0;
    matteExpressionNode_t * prev = NULL;
    while(iter->ttype != MATTE_TOKEN_MARKER_EXPRESSION_END) {
        int preOp = -1;
        int postOp = -1;
        int line = -1;
        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR1) {
            // operator first
            preOp = string_to_operator(iter->text, iter->ttype);
            iter = iter->next;
        }
        line = iter->line;


        // by this point, all complex sub-expressions would have been 
        // reduced, so now we can just work with raw values
        vstartType = iter->ttype;
        matteArray_t * valueInst = compile_value(g, block, functions, &iter, &lvalue);
        if (!valueInst) {
            goto L_FAIL;
        }
        if (preOp != -1) {
            write_instruction__opr(valueInst, line, preOp);
        }
        
        if (iter->ttype == MATTE_TOKEN_GENERAL_OPERATOR2) {
            // operator first
            postOp = string_to_operator(iter->text, iter->ttype);
            iter = iter->next;
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
                if (u.opcode == MATTE_OPCODE_OLK) {
                    isSimpleReferrable = 0;
                    break;
                }
            }

            // Heres the fun part: lvalues are either
            // values that got reduced to a referrable OR an expression dot access OR a table lookup result.
            if (isSimpleReferrable) {
                postOp = POST_OP_SYMBOLIC__ASSIGN_REFERRABLE + assignment_token_to_op_index(iter->ttype);        
                if (undo.opcode != MATTE_OPCODE_PRF) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing referrable token. (internal error)");
                    goto L_FAIL;
                }
                
                if (is_referrable_const(block, *(uint32_t*)(undo.data))) {
                    matte_syntax_graph_print_compile_error(g, iter, "Cannot assign new value to constant.");
                    goto L_FAIL;                    
                }
            } else {
                // for handling assignment for the dot access and the [] lookup, 
                // the OLK instruction will be removed. This leaves both the 
                // object AND the key on the stack (since OLK would normally consume both)
                postOp = POST_OP_SYMBOLIC__ASSIGN_MEMBER + assignment_token_to_op_index(iter->ttype) + (*((uint32_t*)undo.data) ? MATTE_OPERATOR_STATE_BRACKET : 0);
                matte_array_set_size(valueInst, size-1);
                if (undo.opcode != MATTE_OPCODE_OLK) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing lookup token. (internal error)");
                    goto L_FAIL;
                }
            }

            iter = iter->next;
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

    uint32_t i, si;
    uint32_t len = matte_array_get_size(nodes);
    for(i = 0; i < len; ++i) {
        matteExpressionNode_t * n = matte_array_at(nodes, matteExpressionNode_t *, i);

        // one operand always is applied before the 2op (post op)
        if (n->preOp > -1) {
        }

        // for 2-operand instructions, the first node is merged with the second node by 
        // putting instructions to compute it in order. 
        if (n->postOp > -1) {
            if (n->postOp >= POST_OP_SYMBOLIC__ASSIGN_MEMBER) {
                merge_instructions(n->next->value, n->value);
                write_instruction__osn(n->next->value, n->next->line, n->postOp - POST_OP_SYMBOLIC__ASSIGN_MEMBER);                
            } else if(n->postOp >= POST_OP_SYMBOLIC__ASSIGN_REFERRABLE) {
                uint32_t instSize = matte_array_get_size(n->value);
                matteBytecodeStubInstruction_t lastPRF = matte_array_at(n->value, matteBytecodeStubInstruction_t, instSize-1);
                matte_array_set_size(n->value, instSize-1);
                uint32_t referrable;
                memcpy(&referrable, &lastPRF.data[0], sizeof(uint32_t));

                if (lastPRF.opcode != MATTE_OPCODE_PRF) {
                    matte_syntax_graph_print_compile_error(g, iter, "Missing referrable token. (internal error)");
                    goto L_FAIL;
                }
                merge_instructions(n->next->value, n->value);
                write_instruction__arf(n->next->value, n->next->line, referrable, n->postOp - POST_OP_SYMBOLIC__ASSIGN_REFERRABLE);
            } else { // normal case.
            
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
                    marker.lineNumber = n->line;
                    marker.opcode = 0xffff;
                    marker.data[0] = 1;
                    matte_array_push(n->value, marker);
                    hasandor = 1;
                } else if (n->postOp == MATTE_OPERATOR_OR) {
                    matteBytecodeStubInstruction_t marker;
                    marker.lineNumber = n->line;
                    marker.opcode = 0xffff;
                    marker.data[0] = 2;
                    matte_array_push(n->value, marker);                    
                    hasandor = 1;
                }
                // [1] op [2] -> [1 2 op]
                merge_instructions_rev(n->next->value, n->value);
                write_instruction__opr(n->next->value, n->next->line, n->postOp);
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
    }
    #ifdef MATTE_DEBUG
        assert(len && "If len is 0, that means this expression has NO nodes. Which is definitely. Bad.");
    #endif

    merge_instructions(outInst, matte_array_at(nodes, matteExpressionNode_t *, len-1)->value);

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
            if (inst->opcode == 0xffff) {
                switch(inst->data[0]) {
                  case 1: {// AND 
                    inst->opcode = MATTE_OPCODE_SCA;
                    uint32_t skipAmt = len - i - 1;
                    memcpy(inst->data, &skipAmt, sizeof(uint32_t));                    
                    break;
                  }
                  case 2: {// OLD
                    inst->opcode = MATTE_OPCODE_SCO;
                    uint32_t skipAmt = len - i - 1;
                    memcpy(inst->data, &skipAmt, sizeof(uint32_t));                    
                    break;
                  }

                }
            }
        }
    }


    // whew... now cleanup thanks
    // at this point, all nodes "value" attributes have been cleaned and transfered.
    for(i = 0; i < len; ++i) {
        free(matte_array_at(nodes, matteExpressionNode_t *, i));
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
        uint32_t oln = iter->line;
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
      case MATTE_TOKEN_DECLARE_CONST:
        varConst = 1;
      case MATTE_TOKEN_DECLARE: {
        uint32_t oln = iter->line;
        iter = iter->next; // declaration

        uint32_t gl = get_local_referrable(g, block, iter);
        if (gl != 0xffffffff) {
            iter = iter->next;
        } else {
            matteString_t * m = matte_string_create_from_c_str("Could not find local referrable of the given name '%s'", matte_string_get_c_str(iter->text));
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
        uint32_t oln = iter->line;
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
    matteFunctionBlock_t * b = calloc(1, sizeof(matteFunctionBlock_t));
    b->args = matte_array_create(sizeof(matteString_t *));
    b->strings = matte_array_create(sizeof(matteString_t *));
    b->locals = matte_array_create(sizeof(matteString_t *));
    b->local_isConst = matte_array_create(sizeof(int));
    b->instructions = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
    b->captures = matte_array_create(sizeof(matteBytecodeStubCapture_t));
    b->captureNames = matte_array_create(sizeof(matteString_t *));
    b->capture_isConst = matte_array_create(sizeof(int));
    b->stubID = g->functionStubID++;
    b->parent = parent;
    

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
            // args must ALWAYS be variable names 
            iter = iter->next;
            while(iter && iter->ttype == MATTE_TOKEN_VARIABLE_NAME) {
                matteString_t * arg = matte_string_clone(iter->text);
                matte_array_push(b->args, arg);
                #ifdef MATTE_DEBUG__COMPILER
                    printf("  - Argument %d: %s\n", matte_array_get_size(b->args), matte_string_get_c_str(arg));
                #endif
                iter = iter->next;
                
                //
                // typestrict arguments
                //
                if (iter->ttype == MATTE_TOKEN_FUNCTION_TYPESPEC) { // =>
                    iter = iter->next; // skip =>
                    // first argument with type specified. This means we need to insert 
                    // "any" types for the others.
                    if (!b->typestrict_types) {
                        b->typestrict_types = matte_array_create(sizeof(matteBytecodeStubInstruction_t));
                        if (matte_array_get_size(b->args) != 1) {
                            uint32_t i;
                            uint32_t len = matte_array_get_size(b->args)-1;
                            for(i = 0; i < len; ++i) {
                                write_instruction__pto(b->typestrict_types, iter->line, 7); // any;
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
                    write_instruction__pto(b->typestrict_types, iter->line, 7); 
                }   
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
            
            
            //
            // Return value type 
            //
            if (iter->ttype == MATTE_TOKEN_FUNCTION_TYPESPEC) {
                iter = iter->next; // skip =>
                if (!b->typestrict_types) {
                    b->typestrict_types = matte_array_create(sizeof(matteBytecodeStubInstruction_t));

                    uint32_t i;
                    uint32_t len = matte_array_get_size(b->args);
                    for(i = 0; i < len; ++i) {
                        write_instruction__pto(b->typestrict_types, iter->line, 7); // any;
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
                // no return value type, but we're in strict mode. So enter an "any" type.
                write_instruction__pto(b->typestrict_types, iter->line, 7);             
            }
        } 
        if (b->typestrict_types) {
            b->typestrict = matte_array_get_size(b->args) + 1;
        }

        // most situations will require that the block begin '{' token 
        // exists already. This will be true EXCEPT in the toplevel function call (root stub)
        if (iter->ttype != MATTE_TOKEN_FUNCTION_BEGIN) {
            matte_syntax_graph_print_compile_error(g, iter, "Missing function block begin brace. '{'");
            goto L_FAIL;
        }
        iter = iter->next;
    }


    // next find all locals and static strings
    matteToken_t * funcStart = iter;
    while(iter && iter->ttype != MATTE_TOKEN_FUNCTION_END) {
        if (iter->ttype == MATTE_TOKEN_FUNCTION_CONSTRUCTOR) {
            ff_skip_inner_function(&iter);
        } else if (iter->ttype == MATTE_TOKEN_DECLARE ||
            iter->ttype == MATTE_TOKEN_DECLARE_CONST) {
            int isconst = iter->ttype == MATTE_TOKEN_DECLARE_CONST;

            iter = iter->next;
            if (iter->ttype != MATTE_TOKEN_VARIABLE_NAME) {
                matte_syntax_graph_print_compile_error(g, iter, "Expected local variable name.");
                goto L_FAIL;
            }
            
            matteString_t * local = matte_string_clone(iter->text);
            matte_array_push(b->locals, local);
            matte_array_push(b->local_isConst, isconst);
            
            #ifdef MATTE_DEBUG__COMPILER
                printf("  - Local%s %d: %s\n", isconst? "(const)" : "", matte_array_get_size(b->locals), matte_string_get_c_str(local));
            #endif
        } 
        iter = iter->next;
    }

    // reset
    iter = funcStart;



    // now that we have all the locals and args, we can start emitting bytecode.
    if (iter && iter->ttype != MATTE_TOKEN_FUNCTION_END) {
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
    if (iter) {
        write_instruction__nem(b->instructions, iter->line);
        write_instruction__ret(b->instructions, iter->line);
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
    uint32_t len = matte_string_get_length(str);
    WRITE_BYTES(uint32_t, len);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        int32_t ch = matte_string_get_char(str, i);
        WRITE_BYTES(int32_t, ch);
    }
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

    assert(sizeof(matteBytecodeStubInstruction_t) == sizeof(uint32_t) + sizeof(int32_t) + sizeof(uint64_t));

    for(i = 0; i < len; ++i) {
        matteFunctionBlock_t * block = matte_array_at(arr, matteFunctionBlock_t *, i);
        nSlots = 1;
        WRITE_BYTES(uint8_t, nSlots); // bytecode version
        WRITE_BYTES(uint32_t, block->stubID);
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
        WRITE_NBYTES(nInst * sizeof(matteBytecodeStubInstruction_t), matte_array_get_data(block->instructions));

        function_block_destroy(block);
    }


    *size = matte_array_get_size(byteout);
    uint8_t * out = malloc(*size);
    memcpy(out, matte_array_get_data(byteout), *size);

    matte_array_destroy(byteout);
    matte_array_destroy(arr);
    return out;
}
