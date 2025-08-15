#ifndef _CONTROLLERLIB_API_H
#define _CONTROLLERLIB_API_H

#include <stdint.h>
#include <stdbool.h>
#include <json.h>
#include "../controllerlib_7.xx/controller_nvm.h"

typedef uint32_t (*nvmlib_init_func_t)(void);
typedef void (*nvmlib_term_func_t)(void);
typedef bool (*nvmlib_is_nvm_valid_func_t)(const uint8_t *nvm_ptr, size_t nvm_size, nvmLayout_t nvm_layout);
typedef bool (*nvmlib_nvm_to_json_func_t)(const uint8_t *nvm_buf, size_t nvm_size, json_object **jo_out, nvmLayout_t nvm_layout);
typedef bool (*nvmlib_is_json_valid_func_t)(json_object *jo);
typedef size_t (*nvmlib_json_to_nvm_func_t)(json_object *jo, uint8_t **nvm_buf_ptr, size_t *nvm_size, const char *device_info);
typedef bool (*nvmlib_nvm_to_hex_func_t)(const char *nvm_file, const char *hex_file, uint32_t start_address);
typedef struct
{
  const char *lib_id;
  const char *nvm_desc;
  const char *json_desc;
  nvmlib_init_func_t init;
  nvmlib_term_func_t term;
  nvmlib_is_nvm_valid_func_t is_nvm_valid;
  nvmlib_nvm_to_json_func_t nvm_to_json;
  nvmlib_is_json_valid_func_t is_json_valid;
  nvmlib_json_to_nvm_func_t json_to_nvm;
  nvmlib_nvm_to_hex_func_t nvm_to_hex;
} nvmlib_interface_t;

extern nvmlib_interface_t controller_nvm_if;
#endif // _CONTROLLERLIB_API_H
