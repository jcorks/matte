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


#include "../common/filesystem.common"


