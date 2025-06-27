#ifndef NVMCONV_MAIN_H
#define NVMCONV_MAIN_H
#include <json.h>

json_object *zw_nvm_to_json(const char *in_file,
                            char *out_file,
                            int migration_mode);


json_object *zw_json_to_nvm(const char *in_file,
                            char *out_file,
                            const char *device_info,
                            json_object *input_jo,
                            int migration_mode);
#endif // NVMCONV_MAIN_H