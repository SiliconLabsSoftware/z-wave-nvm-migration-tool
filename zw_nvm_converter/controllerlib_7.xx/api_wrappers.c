#include <stdio.h>
#include <string.h>
#include "controllerlib_api.h"
#include "controller_nvm.h"
#include "bin2hex.h"

/*****************************************************************************/
/* Utility functions */


/*****************************************************************************/
static uint32_t nvmlib711_init(void)
{
  return 0;
}

/*****************************************************************************/
static void nvmlib711_term(void)
{
  // Not implemented
}

/*****************************************************************************/
static bool nvmlib719_nvm_to_hex(const char* nvm_file, const char* hex_file, uint32_t start_address)
{
  return bin2hex(nvm_file, hex_file, start_address);
}

/*****************************************************************************/
bool nvmlib_nvm_to_json(const uint8_t *nvm_image, size_t nvm_image_size, json_object **jo_out, nvmLayout_t nvm_layout){
  if (open_controller_nvm(nvm_image, nvm_image_size, nvm_layout))
  {
    *jo_out = controller_info_nvm_get_json(nvm_layout); 

    close_controller_nvm();

    return true;
  }
  return false;
}

/*****************************************************************************/
static bool nvmlib711_is_json_valid(json_object *jo)
{
  return true;
}

/*****************************************************************************/
static size_t nvmlib_json_to_nvm_common(json_object *jo, uint8_t **nvm_buf_ptr, size_t *nvm_size, const char *device_info)
{
  size_t bin_size = 0;
  uint8_t *buf_ptr = 0;
  nvmLayout_t nvm_layout = NVM3_700s; 
  if (json_get_nvm_layout(device_info, jo, &nvm_layout))
  {
    open_controller_nvm(NULL, 0, nvm_layout);

    bool json_parsed = controller_parse_json(jo, nvm_layout);
    if (true == json_parsed)
    {
      bin_size = get_controller_nvm_image(&buf_ptr, nvm_layout);
    }

    close_controller_nvm();
  }

  if (bin_size > 0)
  {
    *nvm_size    = bin_size;
    *nvm_buf_ptr = buf_ptr;
  }

  return bin_size;
}

nvmlib_interface_t controller_nvm_if = {
  .lib_id        = "NVM Converter for Z-Wave Bridge",
  .nvm_desc      = "bridge_800s_7.22",
  .json_desc     = "JSON",
  .init          = nvmlib711_init,
  .term          = nvmlib711_term,
  .is_nvm_valid  = check_controller_nvm,
  .nvm_to_json   = nvmlib_nvm_to_json,
  .is_json_valid = nvmlib711_is_json_valid,
  .json_to_nvm   = nvmlib_json_to_nvm_common,
  .nvm_to_hex    = nvmlib719_nvm_to_hex
};