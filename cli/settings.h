#ifndef H_MATTE_CLI_SETTINGS
#define H_MATTE_CLI_SETTINGS
typedef struct matte_t matte_t;
typedef struct matteSettings_t matteSettings_t;





typedef struct matteSettings_t matteSettings_t;




// Reads the settings file from 
// the standard system path.
// on Unix-likes, this is in 
// /etc/matte/settings.json
// on Windows, this is [path to exe]/settings.json
// If the settings are missing or otherwise 
// non-existent, NULL is returned.
matteSettings_t * matte_settings_load(matte_t *);


// Gets the path where packages are stored.
// path INCLUDES the trailing slash
const char * matte_settings_get_package_path(matteSettings_t*);


#endif
