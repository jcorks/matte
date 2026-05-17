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

// Implementation of strdup since it is not part of C99 standard
static char * matte_strdup(const char * str) {
    int len = strlen(str);
    char * cpy = matte_allocate(len+1);
    memcpy(cpy, str, len+1);
    return cpy;   
}

MATTE_EXT_FN(matte_filesystem__directoryenumerate) {
    matteStore_t * store = matte_vm_get_store(vm);


    matteArray_t * dirfiles = (matteArray_t*)userData;
    {
        uint32_t i;
        uint32_t len = matte_array_get_size(dirfiles);
        for(i = 0; i < len; ++i) {
            DirectoryInfo * d = &matte_array_at(dirfiles, DirectoryInfo, i);
            matte_deallocate(d->path);
            matte_deallocate(d->name);
        }
        
        matte_array_set_size(dirfiles, 0);
    }


    const int MAXLEN = 32768;
    char * cwd = (char*)matte_allocate(MAXLEN);
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
            info.name = matte_strdup(dent->d_name);
            cwd[len] = 0;
            strcat(cwd, "/");
            strcat(cwd, info.name);
            info.path = matte_strdup(cwd);
            info.isFile = 0;
            if (!stat(info.path, &sinfo)) {
                info.isFile = S_ISREG(sinfo.st_mode);
            }
            matte_array_push(dirfiles, info);
        }
        closedir(d);
    }
    matte_deallocate(cwd);
    

    return matte_store_new_value(store);
}



MATTE_EXT_FN(matte_filesystem__getfullpath) {
    matteStore_t * store = matte_vm_get_store(vm);

    const matteString_t * str = matte_value_string_get_string_unsafe(store, matte_value_as_string(store, args[0]));
    if (!str) {
        matte_vm_raise_error_string(vm, MATTE_VM_STR_CAST(vm, "readBytes() requires the first argument to be string coercible."));
        return matte_store_new_value(store);
    }

    char * canon = realpath(matte_string_get_c_str(str), NULL);
    if (canon == NULL) {
        matte_deallocate(canon);
        return matte_store_new_value(store);
    }
    matteString_t * out = matte_string_create();
    matte_string_concat_printf(out, "%s", canon);
    matte_deallocate(canon);
    matteValue_t outV = matte_store_new_value(store);
    matte_value_into_string(store, &outV, out);
    return outV;
}



#include "../common/filesystem.common"


