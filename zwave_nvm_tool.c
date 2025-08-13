#include "nvmconv_main.h"
#include "libs/json_upgrade/json_upgrade.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zwave_nvm_tool_version.h"

typedef enum
{
  ZW_NVM_TO_JSON_MODE,
  ZW_JSON_TO_NVM_MODE,
  ZW_JSON_UPGRADE_MODE,
  ZW_MIGRATION_MODE,
  UNDEFINE_GLOBAL_MODE
} global_mode_t;

void zw_nvm_migrate(const char *input_file,
                   char *output_file,
                   const char *target_version,
                   const char *device_info,
                   const char *schema_file);
/* Parse the arguments */
global_mode_t parse_main_args(int argc,
                              char **argv,
                              char **file_in,
                              char **file_out,
                              char **target_version,
                              char **device_info,
                              char **schema_file)
{
  int opt;
  global_mode_t global_mode = UNDEFINE_GLOBAL_MODE;
  *file_in = NULL;
  *file_out = NULL;
  *target_version = NULL;
  *device_info = NULL;
  *schema_file = NULL;
  if (argc < 2)
  {
    return UNDEFINE_GLOBAL_MODE;
  }
  while ((opt = getopt(argc, argv, "e:i:u:m:o:h")) != -1)
  {
    switch (opt)
    {
    case 'e':
      global_mode = ZW_NVM_TO_JSON_MODE;
      if (argc < 3)
      {
        fprintf(stderr, "Error: Missing arguments for -e option.\n");
        return UNDEFINE_GLOBAL_MODE;
      }
      *file_in = argv[2];
      break;

    case 'i':
      global_mode = ZW_JSON_TO_NVM_MODE;
      if (argc < 4)
      {
        fprintf(stderr, "Error: Missing arguments for -i option.\n");
        return UNDEFINE_GLOBAL_MODE;
      }
      *file_in = argv[2];
      *device_info = argv[3];
      break;

    case 'u':
      global_mode = ZW_JSON_UPGRADE_MODE;
      if (argc < 5)
      {
        fprintf(stderr, "Error: Missing arguments for -u option.\n");
        return UNDEFINE_GLOBAL_MODE;
      }
      *file_in = argv[2];
      *schema_file = argv[4];
      *target_version = argv[3];
      break;

    case 'm':
      global_mode = ZW_MIGRATION_MODE;
      if (argc < 6)
      {
        fprintf(stderr, "Error: Missing arguments for -m option.\n");
        return UNDEFINE_GLOBAL_MODE;
      }
      *file_in = argv[2];
      *schema_file = argv[5];
      *target_version = argv[3];
      *device_info = argv[4];
      break;

    case 'o':
      if (optarg == NULL)
      {
        fprintf(stderr, "Error: Missing argument for -o option.\n");
        return UNDEFINE_GLOBAL_MODE;
      }
      *file_out = optarg;
      break;

    default: /* '?' */
      return UNDEFINE_GLOBAL_MODE;
    }
  }
  return global_mode;
}

/**
 * @brief Prints the usage instructions for the Z-Wave NVM Migration Tool.
 *
 * This function displays the available options and their descriptions for using
 * the Z-Wave NVM Migration Tool. It provides details on the different modes of
 * operation, including converting between NVM and JSON files, and updating JSON
 * files to a new version using a schema file.
 *
 * @param cmd The name of the command or executable being run.
 * @return Always returns 1.
 */
static int zw_mirgate_tool_print_usage(const char *cmd)
{
  printf("Usage: %s [options] <arguments>\n"
         "Options:\n"
         "    -e  <nvm_file_input>\n"
         "        Convert: READ an NVM file and output data in JSON format.\n"
         "        Example: %s -e input.bin\n\n"
         "    -i <json_file_input> <part_number>\n"
         "        Convert: Parse data from a JSON file (aligned with schema) and generate an NVM file for the specified hardware.\n"
         "        Supported parts: EFR32XG13, EFR32XG14, EFR32XG23, EFR32XG28\n"
         "        Example: %s -i input.json EFR32XG23\n\n"
         "    -u <json_file_input> <target_protocol_version> <schema_file>\n"
         "        Upgrade: Upgrade a JSON data file to a newer version using a schema.\n"
         "        If target_protocol_version == current_protocol_version: populate missing keys with default values.\n"
         "        Example: %s -u input.json 7.19.0 zwave_data_description_scheme.json\n\n"
         "    -m <nvm_file_input> <target_protocol_version> <part_number> <schema_file>\n"
         "        Migration: Update an NVM file to a new version using a schema.\n"
         "        If target_protocol_version == current_protocol_version: populate missing keys with default values.\n"
         "        Example: %s -m input.bin 7.19.0 EFR32XG23 schema.json\n\n"
         "    -o <output_file>\n"
         "        Specify an output file; otherwise, the output file name will be generated automatically.\n"
         "        This option can be used along with other options.\n"
         "        Example: %s -e input.bin -o output.json\n\n"
         "    -h\n"
         "        Display this help message.\n",
         cmd, cmd, cmd, cmd, cmd, cmd);
  return 1;
}

int main(int argc, char **argv)
{
  char *file_out = NULL;
  char *file_in = NULL;
  char *device_info = NULL;
  char *target_version = NULL;
  char *schema_file = NULL;
  global_mode_t global_mode = parse_main_args(argc,
                                              argv,
                                              &file_in,
                                              &file_out,
                                              &target_version,
                                              &device_info,
                                              &schema_file);
  
  printf("Z-Wave NVM Migration Tool Version %s\r\n", ZWAVE_NVM_TOOL_VERSION_STRING);
  switch (global_mode)
  {
  case ZW_NVM_TO_JSON_MODE:
    zw_nvm_to_json(file_in, file_out, false);
    break;

  case ZW_JSON_TO_NVM_MODE:
    zw_json_to_nvm(file_in, file_out, device_info, NULL, false);
    break;

  case ZW_JSON_UPGRADE_MODE:
    upgrade_json_to_version(file_in, file_out, target_version, schema_file, NULL, false);
    break;

  case ZW_MIGRATION_MODE:
    zw_nvm_migrate(file_in, file_out, target_version, device_info, schema_file);
    break;

  case UNDEFINE_GLOBAL_MODE:
    zw_mirgate_tool_print_usage(argv[0]);
    break;

  default:
    break;
  }

  return 0;
}
/**
 * @brief Migrates an NVM file to a new version using a schema.
 *
 * This function performs the following steps:
 * 1. Converts the input NVM file to a JSON object containing all data of the NVM.
 * 2. Upgrades the JSON object to the target protocol version using the provided schema.
 * 3. Converts the upgraded JSON object back to an NVM file for the specified device.
 *
 * @param input_file      Path to the input NVM file.
 * @param output_file     Path to the output NVM file.
 * @param target_version  Target protocol version for migration.
 * @param device_info     Device part number (e.g., EFR32ZG23).
 * @param schema_file     Path to the schema file for upgrading.
 * @return                no return value
 */
void zw_nvm_migrate(const char *input_file,
                   char *output_file,
                   const char *target_version,
                   const char *device_info,
                   const char *schema_file)
{
  json_object *jo = NULL;
  jo = zw_nvm_to_json(input_file, NULL, true);
  upgrade_json_to_version(NULL, NULL, target_version, schema_file, jo, true);
  zw_json_to_nvm(input_file, output_file, device_info, jo, true);
}