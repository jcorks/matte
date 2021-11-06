#ifndef H_MATTE_CLI_SHARED
#define H_MATTE_CLI_SHARED
#include <stdint.h>
#include "../src/matte.h"
#include "../src/matte_vm.h"
#include "../src/matte_bytecode_stub.h"
#include "../src/matte_array.h"
#include "../src/matte_string.h"
#include "../src/matte_compiler.h"
#include "system/system.h"
#include "system/thread.h"

#define MATTE_EXT_FN(__T__) static matteValue_t __T__(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData)


void matte_assemble(char ** assemblyFiles, uint32_t n, const char * output);
void matte_disassemble(char ** assemblyFiles, uint32_t n, const char * output);
int matte_debug(const char * input, char **, int);
void * dump_bytes(const char * filename, uint32_t * len);

#endif
