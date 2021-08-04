#include "../../src/matte_opcode.h"
#include "../../src/matte_string.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>


uint8_t * dump_file(const char * name, uint32_t * sizePtr) {
    FILE * f = fopen(name, "rb");
    char chunk[1024];
    uint32_t chunkSize = 0;
    uint32_t size = 0;
    while((chunkSize = fread(chunk, 1, 1024, f))) {
        size += chunkSize;
    }
    fseek(f, 0, SEEK_SET);
    void * data = malloc(size);
    void * iter = data;
    while((chunkSize = fread(chunk, 1, 1024, f))) {
        memcpy(iter, chunk, chunkSize);
        iter += chunkSize;
    }
    fclose(f);
    *sizePtr = size;
    return data;
}



#define CHOMP(__T__) *(__T__*)(iter); iter += sizeof(__T__); *size -= sizeof(__T__);

static uint32_t  str_len__;
static uint8_t * str_buffer__ = NULL;
static matteString_t * nextString = NULL;
static uint32_t i__;
#define CHOMP_STRING() str_len__ = CHOMP(uint32_t); matte_string_clear(nextString); for(i__=0;i__<str_len__;i__++){int32_t h = CHOMP(int32_t); matte_string_append_char(nextString, h);}



uint8_t * print_function(uint8_t * iter, uint32_t * size) {
    uint32_t id = CHOMP(uint32_t);
    printf("fileid %d\n\n", id);
  
    id = CHOMP(uint32_t);
    printf("function %d\n", id);
    
    // arguments
    uint8_t count = CHOMP(uint8_t);
    printf("  args=%d\n", count);
    uint32_t i;
    for(i = 0; i < count; ++i) {
        CHOMP_STRING();
        printf("    \"%s\"\n", matte_string_get_c_str(nextString));
    }

    // locals
    count = CHOMP(uint8_t);
    printf("  locals=%d\n", count);
    for(i = 0; i < count; ++i) {
        CHOMP_STRING();
        printf("    \"%s\"\n", matte_string_get_c_str(nextString));
    }


    // captures
    count = CHOMP(uint16_t);
    printf("  captures=%d\n", count);
    for(i = 0; i < count; ++i) {
        int a = CHOMP(uint32_t);
        int b = CHOMP(uint32_t);
        printf("    %d %d\n", a, b);
    }



    typedef struct {
        uint8_t bytes[8];
    } InstructionData;
    uint32_t n;
    uint32_t opcodeCount = CHOMP(uint32_t);
    for(i = 0; i < opcodeCount; ++i) {
        uint32_t lineNumber = CHOMP(uint32_t);
        int32_t  opcode = CHOMP(int32_t);        
        uint32_t a;
        uint32_t b;
        InstructionData data = CHOMP(InstructionData);
        printf("  %d ", lineNumber);
        switch(opcode) {
            case MATTE_OPCODE_NOP: 
                printf("nop\n");
                break;
            case MATTE_OPCODE_PRF:
                printf("prf %d\n", *(uint32_t*)data.bytes);
                break;
            case MATTE_OPCODE_NEM:
                printf("nem\n");
                break;

            case MATTE_OPCODE_NBL:
                printf("nbl %d\n", data.bytes[0]);
                break;


            case MATTE_OPCODE_NNM:
                printf("nnm %g\n", *(double*)data.bytes);
                break;

            case MATTE_OPCODE_NST:
                printf("nst \"");
    
                uint32_t len = *(uint32_t*)data.bytes;
                for(n = 0; n < len; ++n) {
                    lineNumber = CHOMP(uint32_t);
                    opcode = CHOMP(int32_t);        
                    data = CHOMP(InstructionData);
                    i++;

                    
                    matteString_t * str = matte_string_create();
                    matte_string_append_char(str, *(int32_t*)data.bytes); n++;
                    if (n < len)
                        matte_string_append_char(str, *(int32_t*)(data.bytes+4));
                    printf("%s", matte_string_get_c_str(str));
                    matte_string_destroy(str);
                }
                
                printf("\"\n");
                break;


            case MATTE_OPCODE_STC:
                break;

            case MATTE_OPCODE_NOB:
                printf("nob\n");
                break;            
            case MATTE_OPCODE_NFN:
                a = *(uint32_t*)data.bytes;
                b = *(uint32_t*)(data.bytes+4);
                
                printf("nfn %d %d\n", a, b);
                break;            

            case MATTE_OPCODE_NAR:
                a = *(uint32_t*)data.bytes;
                printf("nar %d\n", a);
                break;            

            case MATTE_OPCODE_NSO:
                a = *(uint32_t*)data.bytes;
                printf("nso %d\n", a);
                break;            
            

            
            case MATTE_OPCODE_CAL:
                a = *(uint32_t*)data.bytes;
                printf("cal %d\n", a);
                break;            
            
            case MATTE_OPCODE_ARF:
                a = *(uint32_t*)data.bytes;
                printf("arf %d\n", a);
                break;            

            case MATTE_OPCODE_OSN:
                printf("nso\n");
                break;            
                
            case MATTE_OPCODE_OPR:
                printf("opr ");
                switch(data.bytes[0]) {
                    case MATTE_OPERATOR_ADD: printf("+\n"); break; // + 2 operands
                    case MATTE_OPERATOR_SUB: printf("-\n"); break; // - 2 operands
                    case MATTE_OPERATOR_DIV: printf("/\n"); break;
                    case MATTE_OPERATOR_MULT: printf("*\n"); break;
                    case MATTE_OPERATOR_NOT: printf("!\n"); break;
                    case MATTE_OPERATOR_BITWISE_OR: printf("|\n"); break;
                    case MATTE_OPERATOR_OR: printf("||\n"); break;
                    case MATTE_OPERATOR_BITWISE_AND: printf("&\n"); break;
                    case MATTE_OPERATOR_AND: printf("&&\n"); break;
                    case MATTE_OPERATOR_SHIFT_LEFT: printf("<<\n"); break;
                    case MATTE_OPERATOR_SHIFT_RIGHT: printf(">>\n"); break;
                    case MATTE_OPERATOR_POW: printf("**\n"); break;
                    case MATTE_OPERATOR_EQ: printf("==\n"); break;
                    case MATTE_OPERATOR_BITWISE_NOT: printf("~\n"); break;
                    case MATTE_OPERATOR_POINT: printf("->\n"); break;
                    case MATTE_OPERATOR_POUND: printf("#\n"); break;
                    case MATTE_OPERATOR_TERNARY: printf("?\n"); break;
                    case MATTE_OPERATOR_TOKEN: printf("$\n"); break;
                    case MATTE_OPERATOR_GREATER: printf(">\n"); break;
                    case MATTE_OPERATOR_LESS: printf("<\n"); break;
                    case MATTE_OPERATOR_GREATEREQ: printf(">=\n"); break;
                    case MATTE_OPERATOR_LESSEQ: printf("<=\n"); break;
                    case MATTE_OPERATOR_SPECIFY: printf("::\n"); break;
                    case MATTE_OPERATOR_TRANSFORM: printf("<>\n"); break;
                    case MATTE_OPERATOR_NOTEQ: printf("!=\n"); break;
                    case MATTE_OPERATOR_MODULO: printf("%%\n"); break;
                    case MATTE_OPERATOR_CARET: printf("^\n"); break;
                }
                break;
            case MATTE_OPCODE_EXT:
                printf("ext ");
                
                
                uint64_t id = *(uint64_t*)data.bytes;
                switch(id) {
                    case MATTE_EXT_CALL_NOOP:
                        printf("noop\n");
                        break;
                    case MATTE_EXT_CALL_GATE:
                        printf("gate\n");
                        break;
                    case MATTE_EXT_CALL_WHILE:
                        printf("while\n");
                        break;
                    case MATTE_EXT_CALL_FOR:
                        printf("for\n");
                        break;
                    case MATTE_EXT_CALL_FOREACH:
                        printf("foreach\n");
                        break;
                    case MATTE_EXT_CALL_MATCH:
                        printf("match\n");
                        break;
                    case MATTE_EXT_CALL_GETEXTERNALFUNCTION:
                        printf("getExternalFunction\n");
                        break;
                    case MATTE_EXT_CALL_IMPORT:
                        printf("import\n");
                        break;
                    case MATTE_EXT_CALL_TONUMBER:
                        printf("tonumber\n");
                        break;
                    case MATTE_EXT_CALL_TOSTRING:
                        printf("tostring\n");
                        break;
                    case MATTE_EXT_CALL_TOBOOLEAN:
                        printf("toboolean\n");
                        break;
                    case MATTE_EXT_CALL_TYPENAME:
                        printf("typename\n");
                        break;

                }
                break;

            case MATTE_OPCODE_POP:
                a = *(uint32_t*)data.bytes;
                printf("pop %d\n", a);
                break;            

            case MATTE_OPCODE_RET:
                printf("ret\n");
                break;            

            case MATTE_OPCODE_SKP:          
                a = *(uint32_t*)data.bytes;
                printf("skp %d\n", a);
                break;            
            default:
                assert("!unknown opcode encountered");
        }
    }    





    
    printf("end function\n\n");  
    return iter;  
}


int main(int argc, char ** argv) {
    if (argc != 2) {
        printf("b2m inputBytecode\n");
        exit(0);
    }
    nextString = matte_string_create();
    
    uint32_t len;
    uint8_t * iter = dump_file(argv[1], &len);
    
    
    while(len > 0) {
        iter = print_function(iter, &len);
    } 
    
}
