#include "../system.h"

static matteArray_t * dirfiles = NULL;

typedef struct {
    char * name;
    char * path;
    int isFile;
} DirectoryInfo;

MATTE_EXT_FN(matte_filesystem__directoryenumerate) {
    matteHeap_t * heap = matte_vm_get_heap(vm);


    if (!dirfiles) {
        dirfiles = matte_array_create(sizeof(DirectoryInfo));
    } else {
        uint32_t i;
        uint32_t len = matte_array_get_size(dirfiles);
        for(i = 0; i < len; ++i) {
            DirectoryInfo * d = &matte_array_at(dirfiles, DirectoryInfo, i);
            free(d->path);
            free(d->name);
        }
        
        matte_array_set_size(dirfiles, 0);
    }



    const int MAXLEN = 32768;
    char * cwd = malloc(MAXLEN);
    cwd = getcwd(cwd, MAXLEN-1);
    uint32_t len = strlen(cwd);

    struct stat sinfo;
    DIR * d = opendir(".");
    if (d) {
        struct dirent * dent;
        while ((dent = readdir(d))) {
            if (!strcmp(dent->d_name, ".") ||
                !strcmp(dent->d_name, "..")) continue;
            DirectoryInfo info;
            info.name = strdup(dent->d_name);
            cwd[len] = 0;
            strcat(cwd, "/");
            strcat(cwd, info.name);
            info.path = strdup(cwd);
            info.isFile = 0;
            if (!stat(info.path, &sinfo)) {
                info.isFile = S_ISREG(sinfo.st_mode);
            }
            matte_array_push(dirfiles, info);
        }
        closedir(d);
    }
    

    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_filesystem__directoryobjectcount) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    matteValue_t v = matte_heap_new_value(heap);
    if (dirfiles) {
        matte_value_into_number(
            &v,
            matte_array_get_size(dirfiles)
        );
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectname) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    if (matte_vm_pending_message(vm)) return v;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matteString_t * str = matte_string_create_from_c_str(
            matte_array_at(
                dirfiles, 
                DirectoryInfo,
                index
            ).name
        );
        matte_value_into_string(
            &v,
            str
        );
        
        matte_string_destroy(str);
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectpath) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    if (matte_vm_pending_message(vm)) return v;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matteString_t * str = matte_string_create_from_c_str(
            matte_array_at(
                dirfiles, 
                DirectoryInfo,
                index
            ).path
        );
        matte_value_into_string(
            &v,
            str
        );
        
        matte_string_destroy(str);
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectisfile) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    uint32_t index = matte_value_as_number(matte_array_at(args, matteValue_t, 0));

    matteValue_t v = matte_heap_new_value(heap);    
    if (matte_vm_pending_message(vm)) return v;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matte_value_into_boolean(
            &v,
            matte_array_at(
                dirfiles, 
                DirectoryInfo,
                index
            ).isFile
        );
    }
    return v;
}



MATTE_EXT_FN(matte_filesystem__getcwd) {
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

MATTE_EXT_FN(matte_filesystem__setcwd) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("setting the current path requires a path string to an existing directory."));
        return matte_heap_new_value(heap);
    }
    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (value is not string coercible)"));
        return matte_heap_new_value(heap);
    }
    if (chdir(matte_string_get_c_str(str)) == -1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (change directory failed)"));        
        return matte_heap_new_value(heap);
    }
    return matte_heap_new_value(heap);
}

MATTE_EXT_FN(matte_filesystem__cwdup) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (chdir("..") == -1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("Could not update the path (change directory failed)"));        
    }
    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_filesystem__readstring) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readString() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readString() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    uint32_t len;
    uint8_t * bytes = dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
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
    return out;
}

MATTE_EXT_FN(matte_filesystem__readbytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readBytes() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("readBytes() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }

    uint32_t len;
    uint8_t * bytes = dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
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

        matte_value_object_set(out, key, val, 1);

        matte_heap_recycle(key);
        matte_heap_recycle(val);
    }
    free(bytes);
    return out;
}


MATTE_EXT_FN(matte_filesystem__writestring) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    const matteString_t * data = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 1)));
    if (!data) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() requires the second argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeString() could not open output file"));
        return matte_heap_new_value(heap);
    }
    // todo: unicode
    if (!fwrite(matte_string_get_c_str(data), 1, matte_string_get_length(data), f)) {
        fclose(f);
        return matte_heap_new_value(heap);
    }
    fclose(f);

    return matte_heap_new_value(heap);
}



MATTE_EXT_FN(matte_filesystem__writebytes) {
    matteHeap_t * heap = matte_vm_get_heap(vm);
    if (matte_array_get_size(args) < 1) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeBytes() requires the first argument to be a path to a file."));
        return matte_heap_new_value(heap);
    }

    const matteString_t * str = matte_value_string_get_string_unsafe(matte_value_as_string(matte_array_at(args, matteValue_t, 0)));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_STR_CAST("writeBytes() requires the first argument to be string coercible."));
        return matte_heap_new_value(heap);
    }


    matteValue_t obj = matte_array_at(args, matteValue_t, 1);


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
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
            return matte_heap_new_value(heap);
        }
    }
    fclose(f);
    free(chunk);
    return matte_heap_new_value(heap);
}


static void matte_system__filesystem(matteVM_t * vm) {
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_getcwd"),   0, matte_filesystem__getcwd, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_setcwd"),   1, matte_filesystem__setcwd, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_cwdup"),    0, matte_filesystem__cwdup, NULL);


    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_directoryenumerate"),    0, matte_filesystem__directoryenumerate, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_directoryobjectcount"),    0, matte_filesystem__directoryobjectcount, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_directoryobjectname"),    1, matte_filesystem__directoryobjectname, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_directoryobjectpath"),    1, matte_filesystem__directoryobjectpath, NULL);
    matte_vm_set_external_function(vm, MATTE_STR_CAST("__matte_::filesystem_directoryobjectisfile"),    1, matte_filesystem__directoryobjectisfile, NULL);

}

