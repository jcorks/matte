#include "../../src/matte.h"
#include "../../src/matte_array.h"

#include <stdlib.h>
#include <stdio.h>
#define TEST_COUNT 1024
#define TEST_LENGTH_SMALL_MIN 11
#define TEST_LENGTH_SMALL_MAX 200

#define TEST_LENGTH_MEDIUM_MIN 100
#define TEST_LENGTH_MEDIUM_MAX 2000

#define TEST_LENGTH_LARGE_MIN 10000
#define TEST_LENGTH_LARGE_MAX 999999

#define BINARY_HEADER_VERSION 1

uint8_t header[] = {
    'M', 'A', 'T', 0x01, 0x06, 'B', BINARY_HEADER_VERSION
};


int get_rng_int(int min, int max) {
    float r = (rand() / (float)RAND_MAX);
    return (max - min)*r + min;
}


static int do_test(int testN, int max) {
    matte_t * m = matte_create();
    matte_set_io(m, NULL, NULL, NULL);
    
    
    matteArray_t * bytes = matte_array_create(1);
    matte_array_push_n(bytes, header, 7);
    
    uint32_t i;
    for(i = 0; i < max; ++i) {
        uint8_t b = get_rng_int(0, 256);
        matte_array_push(bytes, b);
    }
    printf("RUNNING TEST: %d-%d (%'dB)\n", testN/3, testN%3, max); 
    
    matte_run_bytecode(m, matte_array_get_data(bytes), matte_array_get_size(bytes));
    matte_array_destroy(bytes);
    matte_destroy(m);
    
}



int main() {

    
    
    srand(0xbeeffeee);
    
    uint32_t i;
    for(i = 0; i < TEST_COUNT; ++i) {
        do_test(i*3, get_rng_int(TEST_LENGTH_SMALL_MIN, TEST_LENGTH_SMALL_MAX));
        do_test(i*3+1, get_rng_int(TEST_LENGTH_MEDIUM_MIN, TEST_LENGTH_MEDIUM_MAX));
        do_test(i*3+2, get_rng_int(TEST_LENGTH_LARGE_MIN, TEST_LENGTH_LARGE_MAX));
    }
    
    return 0;
}
