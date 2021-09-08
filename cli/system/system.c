#include "../shared.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#define MATTE_EXT_FN(__T__) static matteValue_t __T__(matteVM_t * vm, matteValue_t fn, matteArray_t * args, void * userData)

MATTE_EXT_FN(matte_cli__system_print) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) return matte_heap_new_value(heap);

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (str) {
        printf("%s", matte_string_get_c_str(str));
        fflush(stdout);
    }
    return matte_heap_new_value(heap);
}



#define GETLINE_SIZE 4096

MATTE_EXT_FN(matte_cli__system_getline) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    char * buffer = malloc(GETLINE_SIZE+1);
    buffer[0] = 0;

    fgets(buffer, GETLINE_SIZE, stdin);

    matteValue_t v =  matte_heap_new_value(heap);
    matte_value_into_string(&v, MATTE_STR_CAST(buffer));
    free(buffer);
    return v;    
}

MATTE_EXT_FN(matte_cli__system_clear) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    printf("\x1b[2J");
    fflush(stdout);
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_cli__system_time) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    time_t t = time(NULL);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(&v, t);
    return v;
}


MATTE_EXT_FN(matte_cli__system_readstring) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readString() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readString() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    uint32_t len;
    uint8_t * bytes = dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
        matte_string_destroy(str);
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readString() could not read file."));
        return matte_heap_new_value(heap);
    }


    matteString_t * compiled = matte_string_create();
    uint32_t i;
    for(i = 0; i < len; ++i) {
        // TODO: unicode
        matte_string_append_char(compiled, bytes[i]);
    }
    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_string(&out, compiled);
    free(bytes);
    matte_string_destroy(compiled);
    matte_string_destroy(str);
    return out;
}

MATTE_EXT_FN(matte_cli__system_readbytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readBytes() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readBytes() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    uint32_t len;
    uint8_t * bytes = dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
        matte_string_destroy(str);
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readBytes() could not read file."));
        return matte_heap_new_value(heap);
    }

    matteValue_t out = matte_heap_new_value(heap);
    matte_value_into_new_object_ref(&out);

    uint32_t i;
    for(i = 0; i < len; ++i) {
        matteValue_t key = matte_heap_new_value(heap);
        matteValue_t val = matte_heap_new_value(heap);

        matte_value_into_number(&key, i);
        matte_value_into_number(&val, bytes[i]);

        matte_value_object_set(out, key, val);

        matte_heap_recycle(key);
        matte_heap_recycle(val);
    }
    free(bytes);
    matte_string_destroy(str);
    return out;
}


MATTE_EXT_FN(matte_cli__system_writestring) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    matteString_t * data = matte_value_as_string(matte_array_at(args, matteValue_t, 1));
    if (!data) {
        matte_string_destroy(str);
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the second argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_string_destroy(str);
        matte_string_destroy(data);
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() could not open output file"));
        return matte_heap_new_value(heap);
    }
    // todo: unicode
    if (!fwrite(matte_string_get_c_str(data), 1, matte_string_get_length(data), f)) {
        fclose(f);
        matte_string_destroy(str);
        matte_string_destroy(data);
        return matte_heap_new_value(heap);
    }
    fclose(f);

    matte_string_destroy(str);
    matte_string_destroy(data);
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_cli__system_writebytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeBytes() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeBytes() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    matteValue_t obj = matte_array_at(args, matteValue_t, 1);


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_string_destroy(str);
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeBytes() could not open output file"));
        return matte_heap_new_value(heap);
    }

    uint32_t len = matte_value_object_get_key_count(obj);
    uint32_t i, n;
    uint8_t * chunk = malloc(4096);
    uint32_t chunkSize;

    for(i = 0; i < len;) {
        for(n = 0; n < 4096 && i < len; ++n, ++i) {
            // TODO: unsafe
            chunk[n] = matte_value_as_number(*matte_value_object_array_at_unsafe(obj, i));
        }

        // todo: unicode
        if (!fwrite(chunk, 1, n, f)) {
            fclose(f);
            free(chunk);
            matte_string_destroy(str);
            return matte_heap_new_value(heap);
        }
    }
    fclose(f);
    free(chunk);
    matte_string_destroy(str);
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_cli__system_exit) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("exit() requires at least one argument"));
        return matte_heap_new_value(heap);
    }
    exit(matte_value_as_number(matte_array_at(args, matteValue_t, 0)));
    return matte_heap_new_value(heap);
}


#ifdef _POSIX_SOURCE
#include <unistd.h>
#include <sys/time.h>
MATTE_EXT_FN(matte_cli__system_sleepms) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Sleep requires a number argument"));
        return matte_heap_new_value(heap);
    }


    usleep(1000*matte_value_as_number(matte_array_at(args, matteValue_t, 0)));
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_cli__system_getcwd) {
    const int MAXLEN = 32768;
    char * path = malloc(MAXLEN);
    path[MAXLEN-1] = 0;
    matteHeap_t * heap = matte_vm_get_heap(vm);
    char * cwd = getcwd(path, MAXLEN-1);
    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_string(&v, MATTE_STR_CAST(cwd));
    free(path);
    return v;
}

MATTE_EXT_FN(matte_cli__system_setcwd) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("setting the current path requires a path string to an existing directory."));
        return matte_heap_new_value(heap);
    }
    matteString_t * str = matte_value_as_string(matte_array_at(args, matteValue_t, 0));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (value is not string coercible)"));
        return matte_heap_new_value(heap);
    }
    if (chdir(matte_string_get_c_str(str)) == -1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (change directory failed)"));        
        matte_string_destroy(str);
        return matte_heap_new_value(heap);
    }
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_cli__system_cwdup) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (chdir("..") == -1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (change directory failed)"));        
    }
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_cli__system_getticks) {
    matteHeap_t * heap = matte_vm_get_heap(vm);

    struct timeval t; 
    gettimeofday(&t, NULL);

    matteValue_t v = matte_heap_new_value(heap);
    matte_value_into_number(&v, t.tv_sec*1000LL + t.tv_usec/1000);
    return v;
}

#elif _WIN32

#endif



/*


*/

void matte_vm_add_system_symbols(matteVM_t * vm) {
    // stdlib only (system agnostic)
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_print"),   1, matte_cli__system_print,   NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getline"), 0, matte_cli__system_getline, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_clear"),   0, matte_cli__system_clear, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_readstring"),   1, matte_cli__system_readstring, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_readbytes"),    1, matte_cli__system_readbytes, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_writestring"),  2, matte_cli__system_writestring, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_writebytes"),   2, matte_cli__system_writebytes, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_exit"),   1, matte_cli__system_exit, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_time"),   0, matte_cli__system_time, NULL);


    // OS specific
    
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_sleepms"),   1, matte_cli__system_sleepms, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getcwd"),   0, matte_cli__system_getcwd, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_setcwd"),   1, matte_cli__system_setcwd, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_cwdup"),    0, matte_cli__system_cwdup, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_getticks"),    0, matte_cli__system_getticks, NULL);

    /*
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_directoryenumerate"),    0, matte_cli__system_directoryenumerate, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_directoryobjectcount"),    0, matte_cli__system_directoryobjectcount, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_directoryobjectname"),    1, matte_cli__system_directoryobjectname, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_directoryobjectpath"),    1, matte_cli__system_directoryobjectpath, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("system_directoryobjectisfile"),    1, matte_cli__system_directoryobjectisfile, NULL);
    */
}