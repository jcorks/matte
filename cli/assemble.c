#include "../src/matte_opcode.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

#define MAX_LEN 256
#define FALSE 0
#define TRUE 1


static uint32_t fileid = 1;
static uint32_t lineN = 0;    
static char * line;
static FILE * out;



static int next_line(FILE * f) {
    static char memory[MAX_LEN+1];
    if (fgets(memory, MAX_LEN, f)) {
        line = memory;
        lineN++;
        while(isspace(line[0])) line++;
        uint32_t len = strlen(line);
        if (len) {
            while(isspace(line[len-1]) || line[len-1] == '\r') {line[len-1] = 0; len--;}
        }
        if (line[0] == ';') line[0] = 0;
        return TRUE;
    }
    return FALSE;
}

typedef struct {
    uint32_t len;
    int32_t * data;

} unistring;

static unistring read_unistring_ascii(const char * line) {
    while(isspace(line[0])) line++;
    uint32_t len = strlen(line);
    if (line[0] != '"' || line[len-1] != '"') {
        printf("ERROR on line %d: expected ascii string (starts and ends with \")\n", lineN);
        exit(1);
    }
    len = len-2;
    line++;

    int d;
    unistring out;
    out.len = len;
    out.data = malloc(sizeof(int)*len);
    uint32_t i;
    for(i = 0; i < len; ++i) {
        out.data[i] = *line;
        line++;
    }
    return out;
}

static void function_to_stub(FILE * f, uint32_t id) {
    uint32_t stubid = id;
    uint8_t nargs = 0;
    uint8_t nlocals = 0;
    uint16_t ncaptures = 0;
    uint32_t nstrings = 0;
    unistring * argNames;
    unistring * localNames;
    unistring * strings;
    typedef struct {
        uint32_t stubid;
        uint32_t referrable;
    } captureData;
    captureData * captures;
    int start = ftell(f);

    int argsSet = FALSE;
    int localsSet = FALSE;
    int capturedSet = FALSE;
    int stringsSet = FALSE;
    typedef struct {
        uint32_t line;
        int32_t opcode;
        uint8_t data[8];
    } instruction;

    instruction * instructions = 0;
    uint32_t instructionsCount = 0;
    uint32_t instructionsAlloc = 0;
    uint8_t bytecodeVersion = 1;


    while(next_line(f)) {
        if (line[0] == 0) continue; // empty
        if (!strncmp(line, "strings", strlen("strings"))) {
            if (sscanf(line, "strings=%"SCNu32"", &nstrings) != 1) {
                printf("ERROR on line %d: unable to parse string count. Syntax: strings=[0-UINT32_MAX]\n", lineN);
                exit(1);
            }


            strings = malloc(sizeof(unistring)*nstrings);
            uint32_t i;
            for(i = 0; i < nstrings; ++i) {
                if(!next_line(f)) {
                    printf("ERROR on line %d: ended early (strings expected)\n", lineN);
                    exit(1);                    
                }
                strings[i] = read_unistring_ascii(line);
            }

            stringsSet = TRUE;
        }  else if (!strncmp(line, "args", strlen("args"))) {
            if (sscanf(line, "args=%"SCNu8"", &nargs) != 1) {
                printf("ERROR on line %d: unable to parse arg count. Syntax: arg=[0-255]\n", lineN);
                exit(1);
            }


            argNames = malloc(sizeof(unistring)*nargs);
            uint32_t i;
            for(i = 0; i < nargs; ++i) {
                if(!next_line(f)) {
                    printf("ERROR on line %d: ended early\n", lineN);
                    exit(1);                    
                }
                argNames[i] = read_unistring_ascii(line);
            }

            argsSet = TRUE;
        } else if (!strncmp(line, "locals", strlen("locals"))) {
            if (sscanf(line, "locals=%"SCNu8"", &nlocals) != 1) {
                printf("ERROR on line %d: unable to parse local count. Syntax: locals=[0-255]\n", lineN);
                exit(1);
            }


            localNames = malloc(sizeof(unistring)*nlocals);
            uint32_t i;
            for(i = 0; i < nlocals; ++i) {
                if(!next_line(f)) {
                    printf("ERROR on line %d: ended early (local names expected)\n", lineN);
                    exit(1);                    
                }
                localNames[i] = read_unistring_ascii(line);
            }

            localsSet = TRUE;
        } else if (!strncmp(line, "captures", strlen("captures"))) {
            if (sscanf(line, "captures=%"SCNu16"", &ncaptures) != 1) {
                printf("ERROR on line %d: unable to parse arg count. Syntax: captures=[0-255]\n", lineN);
                exit(1);
            }


            captures = malloc(sizeof(captureData)*ncaptures);
            uint32_t i;
            for(i = 0; i < ncaptures; ++i) {
                if(!next_line(f)) {
                    printf("ERROR on line %d: ended early (captures expected)\n", lineN);
                    exit(1);                    
                }
                if (sscanf(line, "%"SCNu32" %"SCNu32, &(captures[i].stubid), &(captures[i].referrable)) != 2) {
                    printf("ERROR on line %d: unable to parse capture. Syntax: [function/stub id] [referrable index]\n", lineN);
                    exit(1);                    
                }
            }

            capturedSet = TRUE;
        } else if (!strncmp(line, "end function", strlen("end function"))) {
            // NOW we can dump to file.
            if (!argsSet) {
                printf("ERROR in function %d: missing argument specification\n", stubid);
                exit(1);                    
            }
            if (!localsSet) {
                printf("ERROR in function %d: missing locals specification\n", stubid);
                exit(1);                    
            }
            if (!capturedSet) {
                printf("ERROR in function %d: missing captures specification\n", stubid);
                exit(1);                    
            }

            uint32_t i;
            fwrite(&bytecodeVersion, 1, sizeof(uint8_t), out);
            fwrite(&stubid, 1, sizeof(uint32_t), out);
            fwrite(&nargs, 1, sizeof(uint8_t), out);
            for(i = 0; i < nargs; ++i) {
                fwrite(&argNames[i].len, 1, sizeof(uint32_t), out);
                fwrite(argNames[i].data, argNames[i].len, sizeof(int32_t), out);
                free(argNames[i].data);
            }
            free(argNames);
            fwrite(&nlocals, 1, sizeof(uint8_t), out);
            for(i = 0; i < nlocals; ++i) {
                fwrite(&localNames[i].len, 1, sizeof(uint32_t), out);
                fwrite(localNames[i].data, localNames[i].len, sizeof(int32_t), out);
                free(localNames[i].data);
            }
            free(localNames);
            fwrite(&nstrings, 1, sizeof(uint32_t), out);
            for(i = 0; i < nstrings; ++i) {
                fwrite(&strings[i].len, 1, sizeof(uint32_t), out);
                fwrite(strings[i].data, strings[i].len, sizeof(int32_t), out);
                free(strings[i].data);
            }
            free(strings);

            fwrite(&ncaptures, 1, sizeof(uint16_t), out);
            fwrite(captures, ncaptures, sizeof(captureData), out);
            free(captures);

            fwrite(&instructionsCount, 1, sizeof(uint32_t), out);
            fwrite(instructions, instructionsCount, sizeof(instruction), out);
            free(instructions);

            printf("Assembled fn %5d: args?%3d locals?%3d strings?%5d captures?%5d.Ops: %d\n",
                (int)stubid,
                nargs,
                nlocals,
                nstrings,
                ncaptures,
                instructionsCount
            );
            fflush(stdout);
            return;

        } else { // opcode
            char oc0, oc1, oc2;
            if (instructionsAlloc == instructionsCount) {
                instructionsAlloc = 10+1.4*instructionsAlloc;
                instructions = realloc(instructions, instructionsAlloc*sizeof(instruction));
            }
            instruction * inst = instructions+(instructionsCount++);
            if (sscanf(line, "%"SCNu32" %c%c%c", &inst->line, &oc0, &oc1, &oc2) != 4) {
                printf("ERROR on line %d: unrecognized opcode format! needs to be: [line number] [3-char lowercase opcode] [args]\n", lineN);
                exit(1);
            }
            inst->opcode = 255;
            if (oc0 == 'n' &&
                oc1 == 'o' &&
                oc2 == 'p') {
                inst->opcode = MATTE_OPCODE_NOP;
            } else if (
                oc0 == 'p' &&
                oc1 == 'r' &&
                oc2 == 'f'
            ) {
                inst->opcode = MATTE_OPCODE_PRF;
                if (sscanf(line, "%"SCNu32" prf %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized prf format. Syntax: [line] prf [ref id]\n", lineN);
                    exit(1);
                };                
            } else if (
                oc0 == 'n' &&
                oc1 == 'e' &&
                oc2 == 'm'
            ) {
                inst->opcode = MATTE_OPCODE_NEM;

            } else if (
                oc0 == 'n' &&
                oc1 == 'n' &&
                oc2 == 'm'
            ) {
                inst->opcode = MATTE_OPCODE_NNM;
                if (sscanf(line, "%"SCNu32" nnm %lf", &inst->line, (double*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized nnm format. Syntax: [line] nnm [decimal]\n", lineN);
                    exit(1);
                }               
            } else if (
                oc0 == 'n' &&
                oc1 == 'b' &&
                oc2 == 'l'
            ) {
                inst->opcode = MATTE_OPCODE_NBL;
                int val;
                if (sscanf(line, "%"SCNu32" nbl %d", &inst->line, &val) != 2) {
                    printf("ERROR on line %d: unrecognized nbl format. Syntax: [line] nbl [1 or 0]\n", lineN);
                    exit(1);
                }   
                inst->data[0] = val;
            } else if (
                oc0 == 'n' &&
                oc1 == 's' &&
                oc2 == 't'
            ) {
                inst->opcode = MATTE_OPCODE_NST;
                int val;
                if (sscanf(line, "%"SCNu32" nst %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized nst format. Syntax: [line] nst [string id]\n", lineN);
                    exit(1);
                }   
            } else if (
                oc0 == 'n' &&
                oc1 == 'o' &&
                oc2 == 'b'
            ) {
                inst->opcode = MATTE_OPCODE_NOB;
            } else if (
                oc0 == 'n' &&
                oc1 == 'f' &&
                oc2 == 'n'
            ) {
                inst->opcode = MATTE_OPCODE_NFN;
                if (sscanf(line, "%"SCNu32" nfn %"SCNu32"", &inst->line, (uint32_t*)(&inst->data[4])) != 2) {
                    printf("ERROR on line %d: unrecognized nfn format. Syntax: [line] nfn [stubid]\n", lineN);
                    exit(1);                
                }
            } else if (
                oc0 == 'n' &&
                oc1 == 'a' &&
                oc2 == 'r'
            ) {
                inst->opcode = MATTE_OPCODE_NAR;
                if (sscanf(line, "%"SCNu32" nar %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized nar format. Syntax: [line] nar [number of stack objects into array]\n", lineN);
                    exit(1);                
                }
            } else if (
                oc0 == 'n' &&
                oc1 == 's' &&
                oc2 == 'o'
            ) {
                inst->opcode = MATTE_OPCODE_NSO;
                if (sscanf(line, "%"SCNu32" nso %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized nso format. Syntax: [line] nar [number of object pairs]\n", lineN);
                    exit(1);                
                }
            } else if (
                oc0 == 'p' &&
                oc1 == 't' &&
                oc2 == 'o'
            ) {
                inst->opcode = MATTE_OPCODE_PTO;
                if (sscanf(line, "%"SCNu32" pto %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized pto format. Syntax: [line] pto [type code]\n", lineN);
                    exit(1);                
                }

            } else if (
                oc0 == 's' &&
                oc1 == 'f' &&
                oc2 == 's'
            ) {
                inst->opcode = MATTE_OPCODE_SFS;
                if (sscanf(line, "%"SCNu32" sfs %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized sfs format. Syntax: [line] sfs [type count]\n", lineN);
                    exit(1);                
                }
            }else if (
                oc0 == 'c' &&
                oc1 == 'a' &&
                oc2 == 'l'
            ) {
                inst->opcode = MATTE_OPCODE_CAL;
                if (sscanf(line, "%"SCNu32" cal %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized cal format. Syntax: [line] cal [nargs]\n", lineN);
                    exit(1);                                
                }                
            } else if (
                oc0 == 'a' &&
                oc1 == 'r' &&
                oc2 == 'f'
            ) {
                inst->opcode = MATTE_OPCODE_ARF;
                if (sscanf(line, "%"SCNu32" arf %"SCNu32" %"SCNu32"", &inst->line, (uint32_t*)inst->data, (uint32_t*)(inst->data+4)) != 3) {
                    printf("ERROR on line %d: unrecognized arf format. Syntax: [line] arf [ref index] [asssignment op]\n", lineN);
                    exit(1);                               
                } 

            } else if (
                oc0 == 'o' &&
                oc1 == 's' &&
                oc2 == 'n'
            ) {
                inst->opcode = MATTE_OPCODE_OSN;
                if (sscanf(line, "%"SCNu32" osn %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized osn format. Syntax: [line] osn [assignment op]\n", lineN);
                    exit(1);                                
                }                

            } else if (
                oc0 == 'o' &&
                oc1 == 'l' &&
                oc2 == 'k'
            ) {
                inst->opcode = MATTE_OPCODE_OLK;
                if (sscanf(line, "%"SCNu32" olk %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized osn format. Syntax: [line] olk [is bracket access]\n", lineN);
                    exit(1);                                
                }                

            } else if (
                oc0 == 'o' &&
                oc1 == 'p' &&
                oc2 == 'r'
            ) {
                inst->opcode = MATTE_OPCODE_OPR;
                char opopcode0 = 0;
                char opopcode1 = 0;
                if (sscanf(line, "%"SCNu32" opr %c%c", &inst->line, &opopcode0, &opopcode1) != 3) {
                }



                switch(opopcode0) {
                  case '+': inst->data[0] = MATTE_OPERATOR_ADD; break;
                  case '-': 
                    inst->data[0] = MATTE_OPERATOR_SUB; 
                    switch(opopcode1) {
                      case '>': inst->data[0] = MATTE_OPERATOR_POINT; break;
                      default:;
                    }
                    switch(opopcode1) {
                      case '(': inst->data[0] = MATTE_OPERATOR_NEGATE; break;
                      default:;
                    }


                    break;

                  case '/': inst->data[0] = MATTE_OPERATOR_DIV; break;
                  case '*': 
                    inst->data[0] = MATTE_OPERATOR_MULT;
                    switch(opopcode1) {
                      case '*': inst->data[0] = MATTE_OPERATOR_POW; break;
                      default:;
                    }
                    break;

                  case '!': 
                    inst->data[0] = MATTE_OPERATOR_NOT;
                    switch(opopcode1) {
                      case '=': inst->data[0] = MATTE_OPERATOR_NOTEQ; break;
                      default:;
                    }
                    break;
                  case '|': 
                    inst->data[0] = MATTE_OPERATOR_BITWISE_OR;
                    switch(opopcode1) {
                      case '|': inst->data[0] = MATTE_OPERATOR_OR; break;
                      default:;
                    }
                    break;
                  case '&':
                    inst->data[0] = MATTE_OPERATOR_BITWISE_AND;
                    switch(opopcode1) {
                      case '&': inst->data[0] = MATTE_OPERATOR_AND; break;
                      default:;
                    }
                    break;

                  case '<':
                    inst->data[0] = MATTE_OPERATOR_LESS;
                    switch(opopcode1) {
                      case '<': inst->data[0] = MATTE_OPERATOR_SHIFT_LEFT; break;
                      case '=': inst->data[0] = MATTE_OPERATOR_LESSEQ; break;
                      case '>': inst->data[0] = MATTE_OPERATOR_TRANSFORM; break;
                      default:;
                    }
                    break;

                  case '>':
                    inst->data[0] = MATTE_OPERATOR_GREATER;
                    switch(opopcode1) {
                      case '<': inst->data[0] = MATTE_OPERATOR_SHIFT_RIGHT; break;
                      case '=': inst->data[0] = MATTE_OPERATOR_GREATEREQ; break;
                      default:;
                    }
                    break;

                  case '=':
                    switch(opopcode1) {
                      case '=': inst->data[0] = MATTE_OPERATOR_EQ; break;
                      default:;
                    }
                    break;

                  case '~': inst->data[0] = MATTE_OPERATOR_BITWISE_NOT; break;


                  case '#': inst->data[0] = MATTE_OPERATOR_POUND; break;
                  case '?': inst->data[0] = MATTE_OPERATOR_TERNARY; break;
                  case '$': inst->data[0] = MATTE_OPERATOR_TOKEN; break;
                  case '^': inst->data[0] = MATTE_OPERATOR_CARET; break;
                  case '%': inst->data[0] = MATTE_OPERATOR_MODULO; break;


                  default:
                    printf("ERROR on line %d: unknown opcode\n", lineN);
                    exit(1);
                }
            } else if (
                oc0 == 'e' &&
                oc1 == 'x' &&
                oc2 == 't'
            ) {    
                char * m = malloc(MAX_LEN);
                m[0] = 0;
                sscanf(line, "%"SCNu32" ext %s", &inst->line, m);
                inst->opcode = MATTE_OPCODE_EXT;
                if (!strcmp("noop", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_NOOP;
                } else if (!strcmp("gate", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_GATE;
                } else if (!strcmp("loop", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_LOOP;
                } else if (!strcmp("for", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_FOR;
                } else if (!strcmp("foreach", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_FOREACH;
                } else if (!strcmp("match", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_MATCH;
                } else if (!strcmp("newtype", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_TYPE;
                } else if (!strcmp("instantiate", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_INSTANTIATE;
                } else if (!strcmp("introspect", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_INTROSPECT;
                } else if (!strcmp("getExternalFunction", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_GETEXTERNALFUNCTION;
                } else if (!strcmp("print", m)) {
                    *(uint64_t*)inst->data = MATTE_EXT_CALL_PRINT;
                } else {
                    printf("ERROR on line %d: unrecognized external opcode\n", lineN);
                    exit(1);
                }
                free(m);
            } else if (
                oc0 == 'p' &&
                oc1 == 'o' &&
                oc2 == 'p'
            ) { 
                inst->opcode = MATTE_OPCODE_POP; 
                if (sscanf(line, "%"SCNu32" pop %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized pop format. Syntax: [line] pop [pop count]\n", lineN);
                    exit(1);                
                }
                
            } else if (
                oc0 == 'p' &&
                oc1 == 'n' &&
                oc2 == 'r'
            ) { 
                inst->opcode = MATTE_OPCODE_PNR; 
                if (sscanf(line, "%"SCNu32" pnr %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized pop format. Syntax: [line] pnr [string id]\n", lineN);
                    exit(1);                
                }
                
            } else if (
                oc0 == 'c' &&
                oc1 == 'p' &&
                oc2 == 'y'
            ) { 
                inst->opcode = MATTE_OPCODE_CPY; 
                if (sscanf(line, "%"SCNu32" cpy", &inst->line) != 1) {
                    printf("ERROR on line %d: unrecognized cpy format. Syntax: [line] cpy\n", lineN);
                    exit(1);                
                }
                
            } else if (
                oc0 == 'r' &&
                oc1 == 'e' &&
                oc2 == 't'
            ) { 
                inst->opcode = MATTE_OPCODE_RET; 
                
            } else if (
                oc0 == 's' &&
                oc1 == 'k' &&
                oc2 == 'p'
            ) { 
                inst->opcode = MATTE_OPCODE_SKP; 
                if (sscanf(line, "%"SCNu32" skp %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized skp format. Syntax: [line] skp [pc skip count]\n", lineN);
                    exit(1);                
                }
                
            } else if (
                oc0 == 'a' &&
                oc1 == 's' &&
                oc2 == 'p'
            ) { 
                inst->opcode = MATTE_OPCODE_ASP; 
                if (sscanf(line, "%"SCNu32" asp %"SCNu32"", &inst->line, (uint32_t*)inst->data) != 2) {
                    printf("ERROR on line %d: unrecognized skp format. Syntax: [line] asp [pc skip count]\n", lineN);
                    exit(1);                
                }
                
            }else {
                printf("ERROR on line %d: unrecognized instruction\n", lineN);
                exit(1);
            }
        }
    }
}

void matte_assemble(char ** assemblyFiles, uint32_t n, const char * output) {
    out = fopen(output, "wb"); 
    if (!out) {
        printf("Could not open output file %s\n", output);
        exit(1);
    }


    uint32_t i;
    for(i = 0; i < n; ++i) {
        FILE * f = fopen(assemblyFiles[i], "r");
        if (!f) {
            printf("Could not open input file %s\n", assemblyFiles[i]);
            exit(1);
        }



        while(next_line(f)) {
            // comment
            if (line[0] == 0) continue; // empty
            if (!strncmp(line, "function", strlen("function"))) {
                uint32_t substubid;
                if (sscanf(line, "function %"SCNu32"", &substubid) != 1) {
                    printf("ERROR on line %d: function needs unsigned 16bit number id\n", lineN);
                    exit(1);
                }
                function_to_stub(f, substubid);
            } else {
                printf("ERROR on line %d: parse error (not in function, and no function present?)\n", lineN);
                exit(1);

            }
        }

        fclose(f);
    }
    fclose(out);


}
