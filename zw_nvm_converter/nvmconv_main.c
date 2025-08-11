/* © 2019 Silicon Laboratories Inc. */
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <user_message.h>
#include "controllerlib_api.h"
#include "controllerlib_7.xx/controller_nvm.h"
#include "nvmconv_main.h"

#define NVM3_START_ADDRESS (0x08074000)

/*****************************************************************************/
static size_t load_file_to_buf(const char *fname, uint8_t **buf)
{
  size_t file_size = 0;
  uint8_t *file_buf = NULL;
  size_t items_read = 0;
  // printf("[DEBUG_LOG]: NVM file name: %s\n", fname);
  FILE *fp = fopen(fname, "rb");

  if (NULL != fp)
  {
    // Get file size to allocate enough memory
    // printf("[DEBUG_LOG]: NVM File open SUCCESS!\n");
    if (fseek(fp, 0, SEEK_END) == 0)
    {
      file_size = ftell(fp);
    }

    if (file_size > 0)
    {
      rewind(fp);
      file_buf = malloc(file_size);
      if (NULL != file_buf)
      {
        // Here one "item" is a block of "file_size" bytes, i.e. the whole file
        items_read = fread(file_buf, file_size, 1, fp);
      }
    }
    fclose(fp);
  }
  else
  {
    // printf("[DEBUG_LOG]: Cannot Open NVM File\n");
  }
  if (items_read > 0)
  {
    *buf = file_buf;
    // printf("[DEBUG_LOG]: Read Data from NVM file SUCCESS!\n");
    return file_size;
  }
  // printf("[DEBUG_LOG]: Read Data from NVM file FAIL!\n");
  return 0;
}

/*****************************************************************************/
json_object *zw_nvm_to_json(const char *in_file,
                            char *out_file,
                            int migration_mode)
{
  nvmlib_interface_t *nvm_if = &controller_nvm_if;
  int rc = EXIT_SUCCESS;
  /* EXPORT: Convert BIN to JSON */
  uint8_t *nvm_buf = NULL;
  size_t nvm_size = 0;
  nvmLayout_t nvm_layout = NVM3_700s;
  nvm_size = load_file_to_buf(in_file, &nvm_buf);
  nvm_layout = read_layout_from_nvm_size(nvm_size);
  if (nvm_size > 0)
  {
    nvm_if->init();
    if (nvm_if->is_nvm_valid(nvm_buf, nvm_size, nvm_layout))
    {
      json_object *jo;

      if (nvm_if->nvm_to_json(nvm_buf, nvm_size, &jo, nvm_layout))
      {
        if (migration_mode)
        {
          return jo;
        }
        else
        {
          if (out_file == NULL)
          {
            // Generate out_file name by replacing .bin with .json in in_file
            size_t len = strnlen(in_file, 50);
            if (len > 4 && strcmp(in_file + len - 4, ".bin") == 0)
            {
              out_file = malloc(len - 3 + 5); // Replace ".bin" with ".json"
              if (out_file != NULL)
              {
                strncpy(out_file, in_file, len - 4);
                strncpy(out_file + len - 4, ".json", 5);
                out_file[len + 1] = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
            }
            else
            {
              printf("WARNING: in_file does not have a .bin extension\n"
                     "Using `output.json` as default output file name\n");
              out_file = malloc(12); // "output.json" + null terminator
              if (out_file != NULL)
              {
                strncpy(out_file, "output.json", 11);
                out_file[11] = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
              rc = EXIT_FAILURE;
            }
          }

          if (out_file != NULL)
          {
            printf("NVM data is saved to %s\n", out_file);

            if (json_object_to_file_ext(out_file, jo, JSON_C_TO_STRING_PRETTY) == -1)
            {
              printf("ERROR: %s\n", json_util_get_last_err());
              rc = EXIT_FAILURE;
            }
          }
        }
      }
      else
      {
        printf("Failed to convert nvm to json.\n");
        rc = EXIT_FAILURE;
      }
    }
    else
    {
      printf("ERROR: can not open nvm file\n");
      rc = EXIT_FAILURE;
    }
    nvm_if->term();
  }

  if (nvm_buf)
    free(nvm_buf);
}

/*****************************************************************************/
json_object *zw_json_to_nvm(const char *in_file,
                            char *out_file,
                            const char *device_info,
                            json_object *input_jo,
                            int migration_mode)
{
  nvmlib_interface_t *nvm_if = &controller_nvm_if;
  int rc = EXIT_SUCCESS;
  /* IMPORT: Convert JSON to BIN */
  json_object *jo = NULL;
  if (migration_mode)
  {
    jo = input_jo;
  }
  else
  {
    jo = json_object_from_file(in_file);
  }
  if (jo)
  {
    nvm_if->init();
    if (nvm_if->is_json_valid(jo))
    {
      uint8_t *nvm_buf = NULL;
      size_t nvm_size = 0;

      int ret = nvm_if->json_to_nvm(jo, &nvm_buf, &nvm_size, device_info);
      if (ret > 0 && nvm_size > 0)
      {
        printf("NVM3 binary size: %zu bytes\n", nvm_size);
        if (out_file == NULL)
        {
          // Generate out_file name by replacing .json with .bin in in_file
          size_t len = strlen(in_file);
          if (migration_mode)
          {
            if (len > 3 && strcmp(in_file + len - 4, ".bin") == 0)
            {
              out_file = malloc(len - 4 + 14); // Replace ".json" with ".bin"
              if (out_file != NULL)
              {
                strncpy(out_file, in_file, len - 4);
                strncpy(out_file + len - 4, "_upgraded.bin", 13);
                out_file[len - 4 + 13] = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
            }
            else
            {
              printf("WARNING: in_file does not have a .bin extension\n"
                     "Using `output.json` as default output file name\n");
              out_file = malloc(11); // "output.bin" + null terminator
              if (out_file != NULL)
              {
                strncpy(out_file, "output.bin", 10);
                out_file[10] = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
              rc = EXIT_FAILURE;
            }
          }
          else
          {
            if (len > 3 && strcmp(in_file + len - 5, ".json") == 0)
            {
              out_file = malloc(len - 4 + 4); // Replace ".json" with ".bin"
              if (out_file != NULL)
              {
                strncpy(out_file, in_file, len - 5);
                strncpy(out_file + len - 5, ".bin", 4);
                out_file[len - 1]  = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
            }
            else
            {
              printf("WARNING: in_file does not have a .bin extension\n"
                     "Using `output.json` as default output file name\n");
              out_file = malloc(11); // "output.bin" + null terminator
              if (out_file != NULL)
              {
                strncpy(out_file, "output.bin", 10);
                out_file[10] = '\0'; // Ensure null termination
              }
              else
              {
                printf("ERROR: Memory allocation failed for out_file\n");
                rc = EXIT_FAILURE;
              }
              rc = EXIT_FAILURE;
            }
          }
        }
        printf("NVM image is generated to %s\n", out_file);
        FILE *fp = fopen(out_file, "wb");
        if (fp)
        {
          size_t items_written = fwrite(nvm_buf, nvm_size, 1, fp);
          if (items_written != 1)
          {
            printf("ERROR %d writing to file %s\n", ferror(fp), out_file);
            rc = EXIT_FAILURE;
          }
          fclose(fp);
        }
        else
        {
          printf("ERROR creating file %s\n", out_file);
          rc = EXIT_FAILURE;
        }
      }
      else
      {
        printf("ERROR: Invalid JSON to convert\n");
        rc = EXIT_FAILURE;
      }
    }
    else
    {
      printf("ERROR: Invalid JSON\n");
      rc = EXIT_FAILURE;
    }
    nvm_if->term();
  }
  else
  {
    // json_util_get_last_err() unavailable before json-c 0.13
    // Instead json-c prints directly to stderr
    printf("ERROR: convert json to nvm fail - %s\n", json_util_get_last_err());
    rc = EXIT_FAILURE;
  }
}
