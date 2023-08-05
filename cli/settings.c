#include "settings.h"
#include "../src/matte_string.h"
#include "../src/matte_store.h"
#include "../src/matte_vm.h"
#include "shared.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
    #include <windows.h>
#endif

struct matteSettings_t {
    char * packagePath;
};

// Reads the settings file from 
// the standard system path.
// on Unix-likes, this is in 
// /etc/matte/settings.json
// on Windows, this is [path to exe]/settings.json
// If the settings are missing or otherwise 
// non-existent, NULL is returned.
matteSettings_t * matte_settings_load(matte_t * m) {
    #ifdef __WIN32__
        static char path[512];
        GetModuleFileName(NULL, path, 511);
        char * iter = &path[strlen(path)-1];
        // preserve up to and including last backslash
        while(iter != &(path[0]) && *iter != '\\') iter--;
        if (*iter == '\\')
            iter[1] = 0;
        
    #else 
        const char * path = "/etc/matte/";
    #endif
    
    
    matteString_t * fullpath = matte_string_create_from_c_str("%s%s", path, "settings.json");

    uint32_t dataLen = 0;
    uint8_t * data = (uint8_t *)dump_bytes(matte_string_get_c_str(fullpath), &dataLen, 0);
    matte_string_destroy(fullpath);
    if (!data || ! dataLen) {
        return NULL;
    }

    char * dataString = (char*)matte_allocate(dataLen+1);
    memcpy(dataString, data, dataLen);
    matte_deallocate(data);
    matteString_t * json = matte_string_create_from_c_str("%s", dataString);
    matte_deallocate(dataString);


    // parse json
    matteVM_t * vm = matte_get_vm(m);
    matteStore_t * store = matte_vm_get_store(vm);
    matteValue_t inputJson = matte_store_new_value(store);
    matte_value_into_string(store, &inputJson, json);
    matte_string_destroy(json);
    
    matteValue_t jsonObject = matte_run_source_with_parameters(
        m,
        "return (import(module:'Matte.Core.JSON')).decode(string:parameters);",
        inputJson
    );
    if (matte_value_type(jsonObject) != MATTE_VALUE_TYPE_OBJECT) {
        return NULL;        
    }

    matte_value_object_push_lock(store, jsonObject); // keep
    
    matteSettings_t * settings = (matteSettings_t*)matte_allocate(sizeof(matteSettings_t));
    matteValue_t prop = matte_value_object_access_string(store, jsonObject, MATTE_VM_STR_CAST(vm, "packagePath"));
    if (matte_value_type(prop) == MATTE_VALUE_TYPE_STRING) {
        const matteString_t * str = matte_value_string_get_string_unsafe(store, prop);
        const char * strch = matte_string_get_c_str(str);
        settings->packagePath = (char*)matte_allocate(strlen(strch)+1);
        memcpy(settings->packagePath, strch, strlen(strch));
    } else {
        matte_deallocate(settings);
        return NULL;
    }
    return settings;
}


// Gets the path where packages are stored.
// path INCLUDES the trailing slash
const char * matte_settings_get_package_path(matteSettings_t * settings) {
    return settings->packagePath;
}
