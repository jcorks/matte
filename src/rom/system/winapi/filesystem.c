/*
Copyright (c) 2023, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the Matte project (https://github.com/jcorks/matte)
matte was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/
typedef struct {
    char * name;
    char * path;
    int isFile;
} DirectoryInfo;

MATTE_EXT_FN(matte_filesystem__directoryenumerate) {
    matteStore_t * store = matte_vm_get_store(vm);


    matteArray_t * dirfiles = (matteArray_t*)userData;
    {
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
    char * cwd = (char*)malloc(MAXLEN);
    cwd = getcwd(cwd, MAXLEN-1);
    uint32_t len = strlen(cwd);

    char * cwdsearch = (char*)malloc(MAXLEN + 4);
    cwdsearch[0] = 0;
    strcat(cwdsearch, cwd);
    strcat(cwdsearch, "\\*");

    WIN32_FIND_DATAA sinfo;
    HANDLE * d = (HANDLE*)FindFirstFileA(cwdsearch, &sinfo);
    if (d) {
        while ((FindNextFile(d, &sinfo))) {
            if (!strcmp(sinfo.cFileName, ".") ||
                !strcmp(sinfo.cFileName, "..")) continue;
            DirectoryInfo info;
            info.name = strdup(sinfo.cFileName);
            cwd[len] = 0;
            strcat(cwd, "\\");
            strcat(cwd, info.name);
            info.path = strdup(cwd);
            info.isFile = !(sinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            matte_array_push(dirfiles, info);
        }
        FindClose(d);
    }
    free(cwdsearch);

    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_filesystem__directoryobjectcount) {
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t v = matte_store_new_value(store);
    matteArray_t * dirfiles = (matteArray_t*)userData;
    if (dirfiles) {
        matte_value_into_number(
            store, 
            &v,
            matte_array_get_size(dirfiles)
        );
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectname) {
    matteStore_t * store = matte_vm_get_store(vm);
    uint32_t index = matte_value_as_number(store, args[0]);

    matteValue_t v = matte_store_new_value(store);    
    if (matte_vm_pending_message(vm)) return v;
    matteArray_t * dirfiles = (matteArray_t*)userData;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matteString_t * str = matte_string_create_from_c_str(
            matte_array_at(
                dirfiles, 
                DirectoryInfo,
                index
            ).name
        );
        matte_value_into_string(
            store, 
            &v,
            str
        );
        
        matte_string_destroy(str);
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectpath) {
    matteStore_t * store = matte_vm_get_store(vm);
    uint32_t index = matte_value_as_number(store, args[0]);

    matteValue_t v = matte_store_new_value(store);    
    if (matte_vm_pending_message(vm)) return v;
    matteArray_t * dirfiles = (matteArray_t*)userData;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matteString_t * str = matte_string_create_from_c_str(
            matte_array_at(
                dirfiles, 
                DirectoryInfo,
                index
            ).path
        );
        matte_value_into_string(
            store, 
            &v,
            str
        );
        
        matte_string_destroy(str);
    }
    return v;
}


MATTE_EXT_FN(matte_filesystem__directoryobjectisfile) {
    matteStore_t * store = matte_vm_get_store(vm);
    uint32_t index = matte_value_as_number(store, args[0]);

    matteValue_t v = matte_store_new_value(store);    
    if (matte_vm_pending_message(vm)) return v;
    matteArray_t * dirfiles = (matteArray_t*)userData;

    if (dirfiles && (index < matte_array_get_size(dirfiles))) {
        matte_value_into_boolean(
            store, 
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
    char * path = (char*)malloc(MAXLEN);
    path[MAXLEN-1] = 0;
    matteStore_t * store = matte_vm_get_store(vm);
    char * cwd = getcwd(path, MAXLEN-1);
    matteValue_t v = matte_store_new_value(store);
    matte_value_into_string(store, &v, MATTE_VM_STR_CAST(vm, cwd));
    free(path);
    return v;
}

MATTE_EXT_FN(matte_filesystem__setcwd) {
    matteStore_t * store = matte_vm_get_store(vm);
    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not update the path (value is not string coercible)"));
        return matte_store_new_value(store);
    }
    if (chdir(matte_string_get_c_str(str)) == -1) {
        matteString_t * err = matte_string_create_from_c_str("Could not update the path to %s", matte_string_get_c_str(str));
        matte_vm_raise_error_string(vm, err);
        matte_string_destroy(err);
        return matte_store_new_value(store);
    }
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_filesystem__cwdup) {
    matteStore_t * store = matte_vm_get_store(vm);
    if (chdir("..") == -1) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "Could not update the path above (change directory failed)"));        
    }
    return matte_store_new_value(store);
}


MATTE_EXT_FN(matte_filesystem__readstring) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readString() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    uint32_t len;
    uint8_t * bytes = (uint8_t*)dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readString() could not read file."));
        return matte_store_new_value(store);
    }

    char * bytesStr = (char*)malloc(len+1);
    memcpy(bytesStr, bytes, len);
    free(bytes);
    bytesStr[len] = 0;
    
    matteString_t * compiled = matte_string_create_from_c_str("%s", bytesStr);
    free(bytesStr);
    

    matteValue_t out = matte_store_new_value(store);
    matte_value_into_string(store, &out, compiled);
    matte_string_destroy(compiled);
    return out;
}


MATTE_EXT_FN(matte_filesystem__readjson) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readJSON() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    uint32_t len;
    uint8_t * bytes = (uint8_t*)dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readJSON() could not read file."));
        return matte_store_new_value(store);
    }

    char * bytesStr = (char*)matte_allocate(len+1);
    memcpy(bytesStr, bytes, len);
    matte_deallocate(bytes);
    bytesStr[len] = 0;
    
    matteString_t * compiled = matte_string_create_from_c_str("%s", bytesStr);
    matte_deallocate(bytesStr);
    

    matteValue_t out = matte_json_decode(vm, compiled);
    matte_string_destroy(compiled);
    return out;
}


MATTE_EXT_FN(matte_filesystem__readbytes) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readBytes() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    uint32_t len;
    uint8_t * bytes = (uint8_t*)dump_bytes(matte_string_get_c_str(str), &len);
    if (!len || !bytes) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readBytes() could not read file."));
        return matte_store_new_value(store);
    }

    matteValue_t out = matte_system_shared__create_memory_buffer_from_raw(vm, bytes, len);
    free(bytes);
    return out;
}


MATTE_EXT_FN(matte_filesystem__writestring) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeString() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }


    const matteString_t * data = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[1]));
    if (!data) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeString() requires the second argument to be string coercible."));
        return matte_store_new_value(store);
    }


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeString() could not open output file"));
        return matte_store_new_value(store);
    }
    if (!fwrite(matte_string_get_utf8_data(data), 1, matte_string_get_utf8_length(data), f)) {
        fclose(f);
        return matte_store_new_value(store);
    }
    fclose(f);

    return matte_store_new_value(store);
}



MATTE_EXT_FN(matte_filesystem__writebytes) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeBytes() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }


    matteValue_t obj = args[1];
    uint32_t bsize = 0;
    const uint8_t * buffer = matte_system_shared__get_raw_from_memory_buffer(vm, obj, &bsize);
    if (!buffer) {
        return matte_store_new_value(store);
    }

    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeBytes() could not open output file"));
        return matte_store_new_value(store);
    }


    if (!fwrite(buffer, 1, bsize, f)) {
        fclose(f);
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeBytes() could not complete the write operation. Perhaps insufficient space?"));
        return matte_store_new_value(store);
    }
    fclose(f);
    return matte_store_new_value(store);
}


MATTE_EXT_FN(matte_filesystem__writejson) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeJSON() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }


    matteString_t * data = matte_json_encode(vm, args[1]);
    if (!data) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeJSON() requires the second argument to be string coercible."));
        return matte_store_new_value(store);
    }


    FILE * f = fopen(matte_string_get_c_str(str), "wb");
    if (!f) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "writeJSON() could not open output file"));
        matte_string_destroy(data);
        return matte_store_new_value(store);
    }
    if (!fwrite(matte_string_get_utf8_data(data), 1, matte_string_get_utf8_length(data), f)) {
        fclose(f);
        matte_string_destroy(data);
        return matte_store_new_value(store);
    }
    fclose(f);
    matte_string_destroy(data);
    return matte_store_new_value(store);
}




MATTE_EXT_FN(matte_filesystem__remove) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "remove() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    if (remove(matte_string_get_c_str(str)) != 0) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "remove() could not remove file."));
    }
    return matte_store_new_value(store);
}

MATTE_EXT_FN(matte_filesystem__exists) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    matteValue_t out = matte_store_new_value(store);
    matte_value_into_boolean(store, &out, INVALID_FILE_ATTRIBUTES != GetFileAttributes(matte_string_get_c_str(str)));
    return out;
}


MATTE_EXT_FN(matte_filesystem__getfullpath) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "getFullPath() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    char * canon = (char*)malloc(MAX_PATH+1);
    

    if (!GetFullPathNameA(matte_string_get_c_str(str), MAX_PATH, canon, NULL)) {
        free(canon);
        return matte_store_new_value(store);
    }
    matteString_t * out = matte_string_create();
    matte_string_concat_printf(out, "%s", canon);
    free(canon);
    matteValue_t outV = matte_store_new_value(store);
    matte_value_into_string(store, &outV, out);
    return outV;
}


void matte_system__filesystem_cleanup(matteVM_t * vm, void * v) {
    matteArray_t * dirfiles = (matteArray_t*)v;
    uint32_t i;
    uint32_t len = matte_array_get_size(dirfiles);
    for(i = 0; i < len; ++i) {
        DirectoryInfo * d = &matte_array_at(dirfiles, DirectoryInfo, i);
        free(d->path);
        free(d->name);
    }        
    matte_array_destroy(dirfiles);
}



static void matte_system__filesystem(matteVM_t * vm) {
    matteArray_t * dirfiles = matte_array_create(sizeof(DirectoryInfo));
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_getcwd"),   0, matte_filesystem__getcwd, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_setcwd"),   1, matte_filesystem__setcwd, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_cwdup"),    0, matte_filesystem__cwdup, dirfiles);


    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_directoryenumerate"),    0, matte_filesystem__directoryenumerate, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_directoryobjectcount"),    0, matte_filesystem__directoryobjectcount, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_directoryobjectname"),    1, matte_filesystem__directoryobjectname, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_directoryobjectpath"),    1, matte_filesystem__directoryobjectpath, dirfiles);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_directoryobjectisfile"),    1, matte_filesystem__directoryobjectisfile, dirfiles);


    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_readstring"), 1, matte_filesystem__readstring, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_readjson"), 1, matte_filesystem__readjson, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_readbytes"), 1, matte_filesystem__readbytes, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_writestring"), 2, matte_filesystem__writestring, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_writejson"), 2, matte_filesystem__writejson, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_writebytes"), 2, matte_filesystem__writebytes, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_remove"), 1, matte_filesystem__remove, NULL);
    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_getfullpath"), 1, matte_filesystem__getfullpath, NULL);

    matte_vm_set_external_function_autoname(vm, MATTE_VM_STR_CAST(vm, "__matte_::filesystem_exists"), 1, matte_filesystem__exists, NULL);
    
    matte_vm_add_shutdown_callback(vm, matte_system__filesystem_cleanup, dirfiles);
}

