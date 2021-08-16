#ifndef H_MATTE_CLI_SHARED
#define H_MATTE_CLI_SHARED
#include <stdint.h>
    
void matte_assemble(char ** assemblyFiles, uint32_t n, const char * output);
void matte_disassemble(char ** assemblyFiles, uint32_t n, const char * output);
int matte_debug(const char * input);
void * dump_bytes(const char * filename, uint32_t * len);

#endif
