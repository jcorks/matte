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


static uint16_t fileid = 1;
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
            while(isspace(line[len-1])) line[len-1] = 0;
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
    len-2;
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
}

static const void function_to_stub(FILE * f, uint16_t id) {
    uint16_t stubid = 1;
    uint8_t nargs = 0;
    uint8_t nlocals = 0;
    uint16_t ncaptures = 0;
    unistring * argNames;
    unistring * localNames;
    typedef struct {
        uint16_t stubid;
        uint32_t referrable;
    } captureData;
    captureData * captures;
    int start = ftell(f);

    int argsSet = FALSE;
    int localsSet = FALSE;
    int capturedSet = FALSE;

    typedef struct {
        uint32_t line;
        int32_t opcode;
        uint8_t data[8];
    } instruction;

    instruction * instructions = 0;
    uint32_t instructionsCount = 0;
    uint32_t instructionsAlloc = 0;



    while(next_line(f)) {
        if (line[0] == 0) continue; // empty
        if (!strncmp(line, "function", strlen("function"))) {
            uint16_t substubid;
            if (sscanf(line, "function %"SCNu16"", &substubid) != 1) {
                printf("ERROR on line %d: function needs number unsigned 16bit number id\n", lineN);
                exit(1);
            }
            function_to_stub(f, substubid);

        } else if (!strncmp(line, "args", strlen("args"))) {
            if (sscanf(line, "args=%"SCNu8"", &nargs) != 1) {
                printf("ERROR on line %d: unable to parse arg count. Syntax: arg=[0-255]\n", lineN);
                exit(1);
            }


            argNames = malloc(sizeof(unistring)*nargs);
            uint32_t i;
            for(i = 0; i < nargs; ++i) {
                if(!next_line(f)) {
                    printf("ERROR on line %d: ended early\n", line);
                    exit(1);                    
                }
                argNames[i] = read_unistring_ascii(line);
            }

            argsSet = TRUE;
        } else if (!strncmp(line, "locals", strlen("locals"))) {
            if (sscanf(line, "locals=%"SCNu8"", &stubid) != 1) {
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
                if (sscanf(line, "%"SCNu16" %"SCNu32, &(captures[i].stubid), &(captures[i].referrable)) != 2) {
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
            fwrite(&fileid, 1, sizeof(uint16_t), f);
            fwrite(&stubid, 1, sizeof(uint16_t), f);
            fwrite(&nargs, 1, sizeof(uint8_t), f);
            for(i = 0; i < nargs; ++i) {
                fwrite(&argNames[i].len, 1, sizeof(uint32_t), f);
                fwrite(&argNames[i].data, argNames[i].len, sizeof(int32_t), f);
                free(argNames[i].data);
            }
            free(argNames);
            fwrite(&nlocals, 1, sizeof(uint8_t), f);
            for(i = 0; i < nlocals; ++i) {
                fwrite(&localNames[i].len, 1, sizeof(uint32_t), f);
                fwrite(&localNames[i].data, localNames[i].len, sizeof(int32_t), f);
                free(localNames[i].data);
            }
            free(localNames);
            fwrite(&ncaptures, 1, sizeof(uint16_t), f);
            fwrite(captures, ncaptures, sizeof(captureData), f);
            free(captures);

            fwrite(&instructionsCount, 1, sizeof(uint32_t), f);
            fwrite(&instructions, instructionsCount, sizeof(instruction), f);
            free(instructions);

            printf("Assembled fn %5.d: nargs?%3.d nlocals?%3.d ncaptures?%5.d. Ops: %d\n",
                stubid,
                nargs,
                nlocals,
                ncaptures,
                instructionsCount
            );
            fflush(stdout);


        } else { // opcode
            char oc0, oc1, oc2;
            if (instructionsAlloc == instructionsCount) {
                instructionsAlloc *= 10+1.4*instructionsAlloc;
                instructions = realloc(instructions, instructionsAlloc*sizeof(instruction));
            }
            instruction * inst = instructions+(instructionsCount++);
            sscanf(line, "%"SCNu32" %c%c%c", &inst->line, &oc0, &oc1, &oc2);

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
                sscanf(line, "%"SCNu32" prf %"SCNu32"", &inst->line, inst->data);                
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
                sscanf(line, "%"SCNu32" nnm %d", &inst->line, inst->data);                
            } else if (
                oc0 == 'n' &&
                oc1 == 's' &&
                oc2 == 't'
            ) {
                char * line = strstr(line, "nst");
                unistring str = read_unistring_ascii(line+3);

                inst->opcode = MATTE_OPCODE_NST;
                uint32_t srcline = inst->line;                
                uint32_t lenLeft = str.len;
                uint32_t iter = 0;
                while(lenLeft) {
                    if (instructionsAlloc == instructionsCount) {
                        instructionsAlloc *= 10+1.4*instructionsAlloc;
                        instructions = realloc(instructions, instructionsAlloc*sizeof(instruction));
                    }
                    inst = instructions+(instructionsCount++);
                    inst->opcode = MATTE_OPCODE_NST;
                    inst->line = srcline;
                    memcpy(inst->data, &str.data[iter], sizeof(int32_t));
                    lenLeft--; iter++;
                    if (lenLeft) {
                        memcpy(inst->data+4, &str.data[iter], sizeof(int32_t));
                        lenLeft--; iter++;
                    }
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
                sscanf(line, "%"SCNu32" nfn %"SCNu32" %"SCNu32"", &inst->line, inst->data, inst->data+4);                
            } else if (
                oc0 == 'c' &&
                oc1 == 'a' &&
                oc2 == 'l'
            ) {
                inst->opcode = MATTE_OPCODE_CAL;
                sscanf(line, "%"SCNu32" cal %"SCNu32"", &inst->line, inst->data);                
            } else if (
                oc0 == 'a' &&
                oc1 == 'r' &&
                oc2 == 'f'
            ) {
                inst->opcode = MATTE_OPCODE_ARF;
                sscanf(line, "%"SCNu32" arf %"SCNu32"", &inst->line, inst->data);                
            } else if (
                oc0 == 'o' &&
                oc1 == 's' &&
                oc2 == 'n'
            ) {
                inst->opcode = MATTE_OPCODE_OSN;
            } else if (
                oc0 == 'o' &&
                oc1 == 'p' &&
                oc2 == 'r'
            ) {
                inst->opcode = MATTE_OPCODE_OPR;
                char opopcode0 = 0;
                char opopcode1 = 0;
                sscanf(line, "%"SCNu32" opr %c%c", &inst->line, &opopcode0, &opopcode1);                

                switch(opopcode0) {
                  case '+': inst->data[0] = MATTE_OPERATOR_ADD; break;
                  case '-': 
                    inst->data[0] = MATTE_OPERATOR_SUB; 
                    switch(opopcode1) {
                      case '>': inst->data[0] = MATTE_OPERATOR_CDEREF; break;
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
                      case '=': inst->data[0] = MATTE_OPERATOR_NOTEQUAL; break;
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
                      case '<': inst->data[0] = MATTE_OPERATOR_SHIFT_DOWN; break;
                      case '=': inst->data[0] = MATTE_OPERATOR_LESSEQ; break;
                      case '>': inst->data[0] = MATTE_OPERATOR_TRANSFORM; break;
                      default:;
                    }
                    break;

                  case '>':
                    inst->data[0] = MATTE_OPERATOR_GREATER;
                    switch(opopcode1) {
                      case '<': inst->data[0] = MATTE_OPERATOR_SHIFT_UP; break;
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
                  case 'T':
                    switch(opopcode1) {
                      case 'S': inst->data[0] = MATTE_OPERATOR_TOSTRING; break;
                      case 'N': inst->data[0] = MATTE_OPERATOR_TONUMBER; break;
                      case 'B': inst->data[0] = MATTE_OPERATOR_TOBOOLEAN; break;
                      case 'Y': inst->data[0] = MATTE_OPERATOR_TYPENAME; break;
                      default:;
                    }
                    break;

                  case '#': inst->data[0] = MATTE_OPERATOR_POUND; break;
                  case '?': inst->data[0] = MATTE_OPERATOR_TERNARY; break;
                  case '$': inst->data[0] = MATTE_OPERATOR_CURRENCY; break;
                  case ':':
                    switch(opopcode1) {
                      case ':': inst->data[0] = MATTE_OPERATOR_SPECIFY; break;
                      default:;
                    }
                    break;
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
                    *(uint64_t*)inst->data = 0;
                } else if (!strcmp("if", m)) {
                    *(uint64_t*)inst->data = 1;
                } else if (!strcmp("while", m)) {
                    *(uint64_t*)inst->data = 2;
                } else if (!strcmp("for", m)) {
                    *(uint64_t*)inst->data = 3;
                } else if (!strcmp("foreach", m)) {
                    *(uint64_t*)inst->data = 4;
                } else if (!strcmp("match", m)) {
                    *(uint64_t*)inst->data = 5;
                } else if (!strcmp("getExternalFunction", m)) {
                    *(uint64_t*)inst->data = 6;
                } else if (!strcmp("getExternalValue", m)) {
                    *(uint64_t*)inst->data = 7;
                } else {
                    printf("ERROR on line %d: unrecognized external opcode\n", lineN);
                    exit(1);
                }
            } else if (
                oc0 == 'p' &&
                oc1 == 'o' &&
                oc2 == 'p'
            ) { 
                inst->opcode = MATTE_OPCODE_POP; 
                sscanf(line, "%"SCNu32" pop %"SCNu32"", &inst->line, &inst->data);
                
            } else {
                printf("ERROR on line %d: unrecognized instruction\n", lineN);
                exit(1);
            }
        }
    }
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        printf("usage: m2bc [input Matte assembly] [output raw bytecode]\n");
        return 0;
    }

    FILE * f = fopen(argv[1], "r");
    out = fopen(argv[2], "wb"); 
    if (!f) {
        printf("Could not open input file %s\n", argv[1]);
        return 1;
    }
    if (!out) {
        printf("Could not open output file %s\n", argv[2]);
        return 1;
    }



    while(next_line(f)) {
        // comment
        if (line[0] == 0) continue; // empty
        if (!strncmp(line, "fileid", strlen("fileid"))) {
            if (sscanf(line, "fileid %"SCNu16"", &fileid) != 1) {
                printf("ERROR on line %d: fileid needs number unsigned 16bit number if\n", lineN);
                exit(1);
            }
        } else if (!strncmp(line, "function", strlen("function"))) {
            uint16_t substubid;
            if (sscanf(line, "function %"SCNu16"", &substubid) != 1) {
                printf("ERROR on line %d: function needs unsigned 16bit number id\n", lineN);
                exit(1);
            }
            function_to_stub(f, substubid);
        } else {
            printf("ERROR on line %d: parse error (not in function, and no function present?)\n", lineN);
            exit(1);

        }
    }

    fclose(out);
    fclose(f);



}