





typedef enum {
    MATTE_TOKEN_EMPTY,
    // is a function call or statement 
    // This includes core functions, user functions 
    // being called from a variable 
    MATTE_TOKEN_FUNCTION_CALL_STATEMENT,

    // token is a number literal
    MATTE_TOKEN_LITERAL_NUMBER,
    MATTE_TOKEN_LITERAL_STRING,
    MATTE_TOKEN_ASSIGNMENT,
    MATTE_TOKEN_GENERAL_OPERATOR,
    MATTE_TOKEN_VARIABLE_NAME,
    MATTE_TOKEN_DECLARE,
    MATTE_TOKEN_DECLARE_CONST,
    MATTE_TOKEN_FUNCTION_BEGIN,   
    MATTE_TOKEN_FUNCTION_END,   
    MATTE_TOKEN_FUNCTION_DEFINE,   
    MATTE_TOKEN_FUNCTION_ARG_BEGIN,   
    MATTE_TOKEN_FUNCTION_ARG_END,   
} matteTokenType_t;



typedef struct {
    // type of the token
    matteTokenType_t ttype;
    
    // source line number
    uint32_t line;
    
    // text for the token
    matteString_t * text;
} matteToken_t;


typedef struct matteTextWalker_t matteTextWalker_t;

matteTextWalker_t * matte_text_walker_create(const uint8_t * data);

uint32_t matte_text_walker_current_line(const matteTextWalker_t *);

// skips space and newlines
int matte_text_walker_peek_next(matteTextWalker_t *);

// Given a sequence of matteTokenType_t values (ending with MATTE_TOKEN_EMPTY)
// the walker will register the given sequence as a possible valid 
// "next" chain. These various chains are tried upon the next 
// matte_text_walker_next() call and destroyed after.
void matte_text_walker_add_possible_token_chain(matteTextWalker_t *, ...);

// Walks the given possible token chains and attempts to produce
// tokens for each. The chains are walked in order; if, and only if, a chain 
// is complete are tokens added to the token array. If no possible chain is 
// complete, no tokens are added and -1 is returned.
int matte_text_walker_next(matteTextWalker_t *, matteArray_t * tokens);


int matte_text_walker_is_end(const matteTextWalker_t *);










// parses in function scope
static int matte_compiler__parse_function_scope_statement(
    matteArray_t * tokens;
    matteTextWalker_t * w,
    void(*onError)(const char *, uint32_t)
) {
    matte_text_walker_add_possible_token_chain(
        w,
        MATTE_TOKEN_DECLARE,
        MATTE_TOKEN_VARIABLE_NAME
    );
    matte_text_walker_add_possible_token_chain(
        w,
        MATTE_TOKEN_DECLARE,
        MATTE_TOKEN_VARIABLE_NAME
    );


    switch(matte_text_walker_peek_next(w)) {
      // variable declaration
      case '@':
        matte_text_walker_next_token(w, MATTE_TOKEN_DECLARE, tokens);
        matte_text_walker_next_token(w, MATTE_TOKEN_VARIABLE_NAME, tokens);
        switch(matte_text_walker_peek_next(w)) {
        
        }
        break;

      case '<':
        matte_text_walker_next_token(w, MATTE_TOKEN_DECLARE_CONST, tokens);
        matte_text_walker_next_token(w, MATTE_TOKEN_VARIABLE_NAME, tokens);
        break;

    }
}   



uint8_t * matte_compiler_run(
    const char * source, 
    uint32_t * size
    void(*onError)(const char *, uint32_t)
) {
    matteTextWalker_t * w = matte_text_walker_create(source);

    // use the text walker to generate an array of tokens for 
    // the entire source. Basic case here allows you to 
    // filter out any poorly formed contextual tokens
    matteArray_t * tokens = matte_array_create(sizeof(matteToken_t*));
    while(!matte_text_walker_end(w)) {
        matte_compiler__parse_function_scope(tokens, w, onError);
    } 
    
    // then walk through the tokens and group them together.
    // a majority of compilation errors will likely happen here, 
    // as only a strict series of tokens are allowed in certain 
    // groups. Might want to form function groups with parenting to track referrables.
    
    // finally emit code for groups.
}



/////////////////////////////////////////////////////////////

// walker;

static int utf8_next_char(char ** source) {
    int out;
    if (val < 128) {
        out =(*source)[0];
        *source++;
    } else if (val < 224) {
        out = 0;
        out += (*source)[0];
        out += (*source)[1] * 0xff;
        *source+=2;
    } else if (val < 240) {
        out = 0;
        out += (*source)[0];
        out += (*source)[1] * 0xff;
        out += (*source)[2] * 0xffff;
        *source+=3;
    } else {
        out = 0;
        out += (*source)[0];
        out += (*source)[1] * 0xff;
        out += (*source)[2] * 0xffff;
        out += (*source)[3] * 0xffffff;
        *source+=4;
    }
    return out;
}





