/* © 2019 Silicon Laboratories Inc. */
#ifndef _NVMLIB711_H
#define _NVMLIB711_H

#include <stdbool.h>
#include <json.h>
 
typedef enum {
    NVM3_700s, 
    NVM3_800s_PRIOR_719, 
    NVM3_800s_FROM_719
} nvmLayout_t;

typedef enum {
    ZW_700s, 
    EFR32XG23,
    EFR32XG28
} hardware_info_t;

bool check_controller_v7_18(const uint8_t *nvm_image, size_t nvm_image_size, nvmLayout_t nvm_layout);
bool check_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size, nvmLayout_t nvm_layout);
bool open_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size, nvmLayout_t nvm_layout);
void close_controller_nvm(void);

json_object* controller_nvm711_get_json(void);
json_object* controller_nvm715_get_json(void);
json_object* controller_nvm_800s_from_719_get_json(void);
json_object* controller_info_nvm_get_json(nvmLayout_t nvm_layout);

bool controller_parse_json(json_object *jo,uint32_t target_version);
size_t get_controller_nvm_image(uint8_t **image_buf, nvmLayout_t nvm_layout);

void dump_controller_nvm_keys(void);

json_object* protocol_nvm_get_json(void);

json_object* app_nvm_get_json(void);

nvmLayout_t read_layout_from_nvm_size(size_t nvm_size);
bool json_get_nvm_layout(const char *device_info, json_object *jo, nvmLayout_t *nvm_layout);

#endif // _NVMLIB711_H
