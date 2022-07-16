#include "../src/matte_opcode.h"
#include "../src/matte_string.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "shared.h"





#define CHOMP(__T__) *(__T__*)(iter); iter += sizeof(__T__); *size -= sizeof(__T__);
#define CHOMPN(__P__, __N__) memcpy((void*)__P__, (void*)iter, __N__); iter += __N__; *size -= __N__;

static uint32_t  str_len__;
static uint8_t * str_buffer__ = NULL;
static matteString_t * nextString = NULL;
static uint32_t i__;
#define CHOMP_STRING() str_len__ = CHOMP(uint32_t); matte_string_clear(nextString); for(i__=0;i__<str_len__;i__++){int32_t h = CHOMP(int32_t); matte_string_append_char(nextString, h);}



uint8_t * print_function(FILE * fout, uint8_t * iter, uint32_t * size) {
    uint8_t tag_ver[7];
    CHOMPN(tag_ver, 7);
    if (tag_ver[6] != 1) {
        printf("Function failed tag and version\n");
        exit(0);    
    }

  
    uint32_t id = CHOMP(uint32_t);
    fprintf(fout, "function %d\n", id);
    
    // arguments
    uint8_t count = CHOMP(uint8_t);
    fprintf(fout, "  args=%d\n", count);
    uint32_t i;
    for(i = 0; i < count; ++i) {
        CHOMP_STRING();
        fprintf(fout, "    \"%s\"\n", matte_string_get_c_str(nextString));
    }

    // locals
    count = CHOMP(uint8_t);
    fprintf(fout, "  locals=%d\n", count);
    for(i = 0; i < count; ++i) {
        CHOMP_STRING();
        fprintf(fout, "    \"%s\"\n", matte_string_get_c_str(nextString));
    }

    // strings 
    count = CHOMP(uint32_t);
    fprintf(fout, "  strings=%d\n", (int)count);
    for(i = 0; i < count; ++i) {
        CHOMP_STRING();
        fprintf(fout, "    \"%s\"\n", matte_string_get_c_str(nextString));
    }


    // captures
    count = CHOMP(uint16_t);
    fprintf(fout, "  captures=%d\n", count);
    for(i = 0; i < count; ++i) {
        int a = CHOMP(uint32_t);
        int b = CHOMP(uint32_t);
        fprintf(fout, "    %d %d\n", a, b);
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
        fprintf(fout, "  %d ", lineNumber);
        switch(opcode) {
            case MATTE_OPCODE_NOP: 
                fprintf(fout, "nop\n");
                break;
            case MATTE_OPCODE_PRF:
                fprintf(fout, "prf %d\n", *(uint32_t*)data.bytes);
                break;
            case MATTE_OPCODE_NEM:
                fprintf(fout, "nem\n");
                break;

            case MATTE_OPCODE_NBL:
                fprintf(fout, "nbl %d\n", data.bytes[0]);
                break;


            case MATTE_OPCODE_NNM:
                fprintf(fout, "nnm %g\n", *(double*)data.bytes);
                break;

            case MATTE_OPCODE_NST:
                fprintf(fout, "nst %d\n", *(uint32_t*)data.bytes);
                break;

            case MATTE_OPCODE_NOB:
                fprintf(fout, "nob\n");
                break;            
            case MATTE_OPCODE_NFN:
                a = *(uint32_t*)(&data.bytes[4]);
                
                fprintf(fout, "nfn %d\n", a);
                break;            

            case MATTE_OPCODE_CAA:
                fprintf(fout, "caa\n");
                break;            
            case MATTE_OPCODE_CAS:
                fprintf(fout, "cas\n");
                break;            


            
            case MATTE_OPCODE_CAL:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "cal %d\n", a);
                break;            
            
            case MATTE_OPCODE_ARF:
                a = *(uint32_t*)data.bytes;
                b = *(uint32_t*)(data.bytes+4);
                fprintf(fout, "arf %d %d\n", a, b);
                break;            

            case MATTE_OPCODE_OSN:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "osn %d\n", a);
                break;            

            case MATTE_OPCODE_OLK:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "olk %d\n", a);
                break;            

                
            case MATTE_OPCODE_OPR:
                fprintf(fout, "opr ");
                switch(data.bytes[0]) {
                    case MATTE_OPERATOR_ADD: fprintf(fout, "+\n"); break; // + 2 operands
                    case MATTE_OPERATOR_SUB: fprintf(fout, "-\n"); break; // - 2 operands
                    case MATTE_OPERATOR_DIV: fprintf(fout, "/\n"); break;
                    case MATTE_OPERATOR_MULT: fprintf(fout, "*\n"); break;
                    case MATTE_OPERATOR_NOT: fprintf(fout, "!\n"); break;
                    case MATTE_OPERATOR_BITWISE_OR: fprintf(fout, "|\n"); break;
                    case MATTE_OPERATOR_OR: fprintf(fout, "||\n"); break;
                    case MATTE_OPERATOR_BITWISE_AND: fprintf(fout, "&\n"); break;
                    case MATTE_OPERATOR_AND: fprintf(fout, "&&\n"); break;
                    case MATTE_OPERATOR_SHIFT_LEFT: fprintf(fout, "<<\n"); break;
                    case MATTE_OPERATOR_SHIFT_RIGHT: fprintf(fout, ">>\n"); break;
                    case MATTE_OPERATOR_POW: fprintf(fout, "**\n"); break;
                    case MATTE_OPERATOR_EQ: fprintf(fout, "==\n"); break;
                    case MATTE_OPERATOR_BITWISE_NOT: fprintf(fout, "~\n"); break;
                    case MATTE_OPERATOR_POINT: fprintf(fout, "->\n"); break;
                    case MATTE_OPERATOR_POUND: fprintf(fout, "#\n"); break;
                    case MATTE_OPERATOR_TERNARY: fprintf(fout, "?\n"); break;
                    case MATTE_OPERATOR_TOKEN: fprintf(fout, "$\n"); break;
                    case MATTE_OPERATOR_GREATER: fprintf(fout, ">\n"); break;
                    case MATTE_OPERATOR_LESS: fprintf(fout, "<\n"); break;
                    case MATTE_OPERATOR_GREATEREQ: fprintf(fout, ">=\n"); break;
                    case MATTE_OPERATOR_LESSEQ: fprintf(fout, "<=\n"); break;
                    case MATTE_OPERATOR_TRANSFORM: fprintf(fout, "<>\n"); break;
                    case MATTE_OPERATOR_NOTEQ: fprintf(fout, "!=\n"); break;
                    case MATTE_OPERATOR_MODULO: fprintf(fout, "%%\n"); break;
                    case MATTE_OPERATOR_CARET: fprintf(fout, "^\n"); break;
                    case MATTE_OPERATOR_NEGATE: fprintf(fout, "-()\n"); break;

                }
                break;
            case MATTE_OPCODE_EXT:
                fprintf(fout, "ext ");
                
                
                uint64_t id = *(uint64_t*)data.bytes;
                switch(id) {
                    case MATTE_EXT_CALL_NOOP:
                        fprintf(fout, "noop\n");
                        break;
                    case MATTE_EXT_CALL_FOREVER:
                        fprintf(fout, "forever\n");
                        break;
                    case MATTE_EXT_CALL_IMPORT:
                        fprintf(fout, "import\n");
                        break;
                    case MATTE_EXT_CALL_PRINT:
                        fprintf(fout, "print\n");
                        break;

                    case MATTE_EXT_CALL_SEND:
                        fprintf(fout, "send\n");
                        break;
                    case MATTE_EXT_CALL_ERROR:
                        fprintf(fout, "error\n");
                        break;

                    case MATTE_EXT_CALL_GETEXTERNALFUNCTION:
                        fprintf(fout, "getExternalFunction\n");
                        break;
                        
                    default:
                        fprintf(fout, "???\n");

                }
                break;

            case MATTE_OPCODE_POP:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "pop %d\n", a);
                break;            
            case MATTE_OPCODE_PNR:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "pnr %d\n", a);
                break; 
            case MATTE_OPCODE_QRY:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "qry %d\n", a);
                break; 

            case MATTE_OPCODE_PTO:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "pto %d\n", a);
                break;            

            case MATTE_OPCODE_SFS:
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "sfs %d\n", a);
                break;            


            case MATTE_OPCODE_CPY:
                fprintf(fout, "cpy\n");
                break;
            case MATTE_OPCODE_RET:
                fprintf(fout, "ret\n");
                break;            
            case MATTE_OPCODE_LST:
                fprintf(fout, "lst\n");
                break;            

            case MATTE_OPCODE_SKP:          
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "skp %d\n", a);
                break;  
            case MATTE_OPCODE_SCA:          
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "sca %d\n", a);
                break;  
            case MATTE_OPCODE_SCO:          
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "sco %d\n", a);
                break;  
            case MATTE_OPCODE_ASP:          
                a = *(uint32_t*)data.bytes;
                fprintf(fout, "asp %d\n", a);
                break; 

            default:
                assert("!unknown opcode encountered");
        }
    }    





    
    fprintf(fout, "end function\n\n");  
    return iter;  
}


void matte_disassemble(char ** assemblyFilesSrc, uint32_t n, const char * output) {
    int needsPrint = 0;
    if (n == 0) { // special case: the output file is the imput file, so figure it out       
        assemblyFilesSrc[0] = (char*)output;
        n = 1;
        output = ".output-temporary";
        needsPrint = 1;
    }    

    nextString = matte_string_create();
    uint32_t i;
    FILE * out = fopen(output, "w");
    if (!out) {
        printf("Couldn't open output file %s\n", output);
        exit(1);
    }
    for(i = 0; i < n; ++i) {
        uint32_t len;
        uint8_t * iter = dump_bytes(assemblyFilesSrc[i], &len);
        uint8_t * keep = iter;
        if (!len || !iter) {
            printf("Couldn't open input file %s (or was empty)\n", assemblyFilesSrc[i]);
            exit(1);
        }
        while(len > 0 && iter) {
            iter = print_function(out, iter, &len);

        } 
        free(keep);
    }    
   
    fclose(out);

    if (needsPrint) {
        uint32_t len;
        char * iter = dump_bytes(output, &len);

        char * proper = malloc(len+1);
        proper[len] = 0;
        memcpy(proper, iter, len);
        printf("%s", proper);
        remove(output);        
    }
}
