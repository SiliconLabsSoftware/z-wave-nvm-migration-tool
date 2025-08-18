#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json_upgrade.h>

int compare_versions(const char *v1, const char *v2)
{
  int major1, minor1, patch1;
  int major2, minor2, patch2;

  sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1);
  sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2);

  if (major1 != major2)
    return major1 - major2;
  if (minor1 != minor2)
    return minor1 - minor2;
  return patch1 - patch2;
}

void handle_version_based_addition_removal(json_object *target, json_object *property_schema, const char *key, const char *target_version, int format)
{
  const char *created_version = NULL;
  int created_format = -1;
  int removed_format = -1;

  json_object *created_version_obj;
  if (json_object_object_get_ex(property_schema, "createdVersion", &created_version_obj))
  {
    created_version = json_object_get_string(created_version_obj);
  }

  json_object *created_obj;
  if (json_object_object_get_ex(property_schema, "created", &created_obj))
  {
    created_format = json_object_get_int(created_obj);
  }

  json_object *removed_obj;
  if (json_object_object_get_ex(property_schema, "removed", &removed_obj))
  {
    removed_format = json_object_get_int(removed_obj);
  }

  const char *compare_version = target_version;
  int should_add = 0;

  if (created_format >= 0 && created_format <= format)
  {
    should_add = 1;
  }
  else if (created_version && compare_version)
  {
    if (compare_versions(created_version, compare_version) <= 0)
    {
      should_add = 1;
    }
  }

  if (removed_format >= 0 && format >= removed_format)
  {
    should_add = 0;
  }

  if (should_add)
  {
    if (!json_object_object_get_ex(target, key, NULL))
    {
      json_object *default_value;
      if (json_object_object_get_ex(property_schema, "default_value", &default_value))
      {
        printf("  Adding property: %s with default value.\n", key);
        json_object_object_add(target, key, json_object_get(default_value));
      }
    }
  }
  else
  {
    if (json_object_object_get_ex(target, key, NULL))
    {
      printf("  Removing property: %s as it does not satisfy the criteria.\n", key);
      json_object_object_del(target, key);
    }
  }
}

void apply_schema_changes(json_object *target, json_object *schema, const char *target_version, int format)
{
  // printf("Applying schema changes...\n");

  // Get the "properties" object from the schema
  json_object *properties;
  if (!json_object_object_get_ex(schema, "properties", &properties))
  {
    // fprintf(stderr, "Error: 'properties' not found in schema.\n");
    exit(EXIT_FAILURE);
  }

  // Get the "required" array from the schema
  json_object *required;
  if (json_object_object_get_ex(schema, "required", &required))
  {
    // Iterate over each property in the "required" array
    for (int i = 0; i < json_object_array_length(required); i++)
    {
      const char *key = json_object_get_string(json_object_array_get_idx(required, i));
      // printf("Processing required property: %s\n", key);

      json_object *property_schema;
      if (!json_object_object_get_ex(properties, key, &property_schema))
      {
        // fprintf(stderr, "Error: Property '%s' not found in 'properties'.\n", key);
        continue;
      }

      // Check if the property has type "object"
      json_object *type_obj;
      const char *type = NULL;
      if (json_object_object_get_ex(property_schema, "type", &type_obj))
      {
        type = json_object_get_string(type_obj);
      }

      if (type && strcmp(type, "object") == 0)
      {
        // printf("  Property '%s' is an object. Checking for nested 'required' fields...\n", key);
        json_object *nested_object;
        if (!json_object_object_get_ex(target, key, &nested_object))
        {
          nested_object = json_object_new_object();
          json_object_object_add(target, key, nested_object);
        }
        apply_schema_changes(nested_object, property_schema, target_version, format);
      }
      else if (type && strcmp(type, "array") == 0)
      {
        // printf("  Property '%s' is an array. Checking 'items' type...\n", key);
        json_object *items_obj;
        if (json_object_object_get_ex(property_schema, "items", &items_obj))
        {
          json_object *items_type_obj;
          const char *items_type = NULL;
          if (json_object_object_get_ex(items_obj, "type", &items_type_obj))
          {
            items_type = json_object_get_string(items_type_obj);
          }

          if (items_type && strcmp(items_type, "object") == 0)
          {
            // printf("    'items' type is 'object'. Looping through array indices to apply schema changes...\n");
            json_object *array;
            if (json_object_object_get_ex(target, key, &array) && json_object_get_type(array) == json_type_array)
            {
              for (int i = 0; i < json_object_array_length(array); i++)
              {
                json_object *array_item = json_object_array_get_idx(array, i);
                apply_schema_changes(array_item, items_obj, target_version, format);
              }
            }
          }
          else
          {
            // printf("    'items' type is not 'object'. Treating array as a normal key for version-based addition/removal...\n");
            handle_version_based_addition_removal(target, property_schema, key, target_version, format);
          }
        }
        else
        {
          // printf("    Warning: 'items' field not found for array property '%s'.\n", key);
        }
      }
      else
      {
        handle_version_based_addition_removal(target, property_schema, key, target_version, format);
      }
    }
  }
}

void apply_schema_changes_to_controller(json_object *controller, json_object *schema, const char *target_version, int format)
{
  // printf("Applying schema changes to 'controller'...\n");
  json_object *controller_schema;
  if (json_object_object_get_ex(schema, "properties", &controller_schema) &&
      json_object_object_get_ex(controller_schema, "controller", &controller_schema))
  {
    apply_schema_changes(controller, controller_schema, target_version, format);
    rearrange_keys(controller, controller_schema);
  }
}

void apply_schema_changes_to_nodes(json_object *nodes, json_object *schema, const char *target_version, int format)
{
  // printf("Applying schema changes to 'nodes'...\n");
  json_object *nodes_schema;
  if (json_object_object_get_ex(schema, "properties", &nodes_schema) &&
      json_object_object_get_ex(nodes_schema, "nodes", &nodes_schema) &&
      json_object_object_get_ex(nodes_schema, "patternProperties", &nodes_schema))
  {
    json_object_object_foreach(nodes_schema, pattern_key, pattern_value)
    {
      json_object_object_foreach(nodes, node_id, node_obj)
      {
        // printf("  Processing node: %s\n", node_id);
        apply_schema_changes(node_obj, pattern_value, target_version, format);
        rearrange_keys(node_obj, pattern_value);
      }
    }
  }
}

void apply_schema_changes_to_lrNodes(json_object *lrNodes, json_object *schema, const char *target_version, int format)
{
  // printf("Applying schema changes to 'lrNodes'...\n");
  json_object *lrNodes_schema;
  if (json_object_object_get_ex(schema, "properties", &lrNodes_schema) &&
      json_object_object_get_ex(lrNodes_schema, "lrNodes", &lrNodes_schema) &&
      json_object_object_get_ex(lrNodes_schema, "patternProperties", &lrNodes_schema))
  {
    json_object_object_foreach(lrNodes_schema, pattern_key, pattern_value)
    {
      json_object_object_foreach(lrNodes, node_id, node_obj)
      {
        // printf("  Processing LR node: %s\n", node_id);
        apply_schema_changes(node_obj, pattern_value, target_version, format);
        rearrange_keys(node_obj, pattern_value);
      }
    }
  }
}
int get_app_file_format_from_version(const char *target_version){
  // Derive the app_format from the target version
  int prot_major = 0;
  int prot_minor = 0;
  int prot_patch = 0;
  int app_format = -1;
  if (sscanf(target_version, "%d.%d.%d", &prot_major, &prot_minor, &prot_patch) != 3)
  {
    fprintf(stderr, "Error: Failed to parse target version: %s\n", target_version);
    printf("Target version must use the format <major>.<minor>.<patch>\n");
    exit(EXIT_FAILURE);
  }

  if (prot_major == 7)
  {
    if (prot_minor >= 11 && prot_minor <= 23)
    {
      app_format = 0;
    }
    else if (prot_minor == 24)
    {
      app_format = 1;
    }
    else
    {
      fprintf(stderr, "Unsupported protocol version: %s\n", target_version);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    fprintf(stderr, "Unsupported major version: %d\n", prot_major);
    exit(EXIT_FAILURE);
  }
  return app_format;
}
int get_file_system_format_from_version(const char *target_version)
{
  // Derive the format from the target version
  int prot_major, prot_minor, prot_patch;
  int format = -1;
  if (sscanf(target_version, "%d.%d.%d", &prot_major, &prot_minor, &prot_patch) != 3)
  {
    fprintf(stderr, "Error: Failed to parse target version: %s\n", target_version);
    printf("Target version must use the format <major>.<minor>.<patch>\n");
    exit(EXIT_FAILURE);
  }

  if (prot_major == 7)
  {
    if (prot_minor == 11)
    {
      format = 0;
    }
    else if (prot_minor == 12)
    {
      format = 1;
    }
    else if (prot_minor == 15)
    {
      format = 2;
    }
    else if (prot_minor == 16)
    {
      format = 3;
    }
    else if (prot_minor == 17 || prot_minor == 18)
    {
      format = 4;
    }
    else if (prot_minor >= 19 && prot_minor <= 24)
    {
      format = 5;
    }
    else
    {
      fprintf(stderr, "Unsupported protocol version: %s\n", target_version);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    fprintf(stderr, "Unsupported major version: %d\n", prot_major);
    exit(EXIT_FAILURE);
  }
  return format;
}

void rearrange_keys(json_object *target, json_object *schema)
{
  // printf("Rearranging keys to align with schema order...\n");

  json_object *properties;
  if (!json_object_object_get_ex(schema, "properties", &properties))
  {
    // fprintf(stderr, "Error: 'properties' not found in schema.\n");
    return;
  }

  // Create a new JSON object to store rearranged keys
  json_object *rearranged = json_object_new_object();

  // Iterate over the keys in the schema and copy them in order
  json_object_object_foreach(properties, key, value)
  {
    json_object *property_value;
    if (json_object_object_get_ex(target, key, &property_value))
    {
      // Check if the property is an object or array and handle recursively
      json_object *type_obj;
      const char *type = NULL;
      if (json_object_object_get_ex(value, "type", &type_obj))
      {
        type = json_object_get_string(type_obj);
      }

      if (type && strcmp(type, "object") == 0)
      {
        // printf("  Rearranging nested object: %s\n", key);
        rearrange_keys(property_value, value);
      }
      else if (type && strcmp(type, "array") == 0)
      {
        // printf("  Rearranging array: %s\n", key);
        json_object *items_obj;
        if (json_object_object_get_ex(value, "items", &items_obj))
        {
          if (json_object_get_type(property_value) == json_type_array)
          {
            for (int i = 0; i < json_object_array_length(property_value); i++)
            {
              json_object *array_item = json_object_array_get_idx(property_value, i);
              rearrange_keys(array_item, items_obj);
            }
          }
        }
      }

      json_object_object_add(rearranged, key, json_object_get(property_value));
    }
  }

  // Copy rearranged keys back to the target object
  json_object_object_foreach(target, target_key, target_value)
  {
    json_object_object_del(target, target_key);
  }
  json_object_object_foreach(rearranged, rearranged_key, rearranged_value)
  {
    json_object_object_add(target, rearranged_key, json_object_get(rearranged_value));
  }

  // Free the temporary rearranged object
  json_object_put(rearranged);

  // printf("Rearrangement complete.\n");
}

void upgrade_json_to_version(const char *input_file, char *output_file, const char *target_version, const char *schema_file, json_object *input_jo, int migration_mode)
{
  printf("Starting upgrade process...\n");
  int format = get_file_system_format_from_version(target_version);
  int app_format = get_app_file_format_from_version(target_version);
  json_object *root = NULL;
  // Load the input JSON file
  if (migration_mode)
  {
    root = input_jo;
  }
  else
  {
    printf("Loading input JSON file: %s\n", input_file);
    root = json_object_from_file(input_file);
  }
  if (!root)
  {
    fprintf(stderr, "Error loading JSON file: %s\n", input_file);
    exit(EXIT_FAILURE);
  }

  // Load the schema JSON file
  printf("Loading schema JSON file: %s\n", schema_file);
  json_object *schema = json_object_from_file(schema_file);
  if (!schema)
  {
    fprintf(stderr, "Error loading schema file: %s\n", schema_file);
    json_object_put(root);
    exit(EXIT_FAILURE);
  }

  // Set the format in the root object
  // printf("Setting format to %d in the root object.\n", format);
  json_object_object_add(root, "format", json_object_new_int(format));
  json_object_object_add(root, "applicationFileFormat", json_object_new_int(app_format));

  // Upgrade the "controller" object
  // printf("Upgrading 'controller' object...\n");
  json_object *controller;
  if (json_object_object_get_ex(root, "controller", &controller))
  {
    apply_schema_changes_to_controller(controller, schema, target_version, format);
    json_object_object_add(controller, "applicationVersion", json_object_new_string(target_version));
    json_object_object_add(controller, "protocolVersion", json_object_new_string(target_version));
  }
  else
  {
    printf("'controller' object not found in input JSON.\n");
  }

  // Upgrade each "nodes" object
  // printf("Upgrading 'nodes' objects...\n");
  json_object *nodes;
  if (json_object_object_get_ex(root, "nodes", &nodes))
  {
    apply_schema_changes_to_nodes(nodes, schema, target_version, format);
  }
  else
  {
    printf("'nodes' object not found in input JSON.\n");
  }

  // Upgrade each "lrNodes" object
  // printf("Upgrading 'lrNodes' objects...\n");
  json_object *lrNodes;
  if (json_object_object_get_ex(root, "lrNodes", &lrNodes))
  {
    apply_schema_changes_to_lrNodes(lrNodes, schema, target_version, format);
  }
  else
  {
    printf("'lrNodes' object not found in input JSON.\n");
  }

  if (migration_mode)
  {
    return;
  }
  else
  {
    char temp_output_file[256];
    // Save the updated JSON to the output file
    if (output_file == NULL)
    {
      // Using default output file name "<input_file>_upgraded.json"
      size_t len = strlen(input_file);
      if (len > 5 && strcmp(input_file + len - 5, ".json") == 0)
      {
      snprintf(temp_output_file, sizeof(temp_output_file), "%.*s_upgraded.json", (int)(len - 5), input_file);
      }
      else
      {
      printf("WARNING: input_file does not have a .json extension\n"
           "Using `output_upgraded.json` as default output file name\n");
      snprintf(temp_output_file, sizeof(temp_output_file), "output_upgraded.json");
      }
      output_file = temp_output_file;
    }
    printf("Saving updated JSON to output file: %s\n", output_file);
    if (json_object_to_file_ext(output_file, root, JSON_C_TO_STRING_PRETTY) != 0)
    {
      fprintf(stderr, "Error writing to output file: %s\n", output_file);
      json_object_put(root);
      exit(EXIT_FAILURE);
    }
    printf("Upgrade process completed successfully.\nOutput saved to: %s\n", output_file);
  }
  // Clean up
  json_object_put(root);
  json_object_put(schema);
  return;
}
