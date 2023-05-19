#include "settings.h"

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
matteSettings_t * matte_settings_load(matte_t *) {
    #ifdef __WIN32__
        static char path[512];
        GetModuleFileName(NULL, path, 511);
        char * iter = path[strlen(path)-1];
        // preserve up to and including last backslash
        while(iter != &(path[0]) && *iter != '\\') iter--;
        if (*iter == '\\')
            iter[1] = 0;
        
    #else 
        const char * path = "/etc/matte/";
    #endif
    
    
    matteString_t * fullpath = matte_string_create_from_c_str("%s%s", path, "settings.json");

    uint32_t dataLen = 0;
    uint8_t * data = (uint8_t *)dump_bytes(matte_string_get_c_str(fullpath), &dataLen);
    matte_string_destroy(fullpath);
    if (!data || ! dataLen) {
        return NULL;
    }


    // parse json
    matteStore_t * store = matte_vm_get_store(m->vm);
    matteValue_t inputJson = matte_store_new_value(store);
    matte_value_into_string(store, &inputJson, json);
    matte_string_destroy(json);
    
    matteValue_t jsonObject = matte_run_source_with_parameters(
        m,
        "return (import(module:'Matte.Core.JSON')).decode(string:parameters);",
        inputJson
    );
    matte_value_object_push_lock(store, jsonObject); // keep







    matteSettings_t * settings = matte_allocate(sizeof(matteSettings_t));
    settings->packagePath = 
}


// Gets the path where packages are stored.
// path INCLUDES the trailing slash
const char * matte_settings_get_package_path(matteSettings_t*);
