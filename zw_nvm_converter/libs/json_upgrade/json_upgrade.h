#ifndef JSON_UPGRADE_H 
#define JSON_UPGRADE_H

#include <json.h>

int compare_versions(const char *v1, const char *v2);
void populate_missing_required_properties(json_object *target, json_object *schema);
void apply_schema_changes(json_object *target, json_object *schema, const char *target_version, int format);
void apply_schema_changes_to_controller(json_object *controller, json_object *schema, const char *target_version, int format);
void apply_schema_changes_to_nodes(json_object *nodes, json_object *schema, const char *target_version, int format);
void apply_schema_changes_to_lrNodes(json_object *lrNodes, json_object *schema, const char *target_version, int format); 
json_object* upgrade_json_to_version(const char *input_file, char *output_file, const char *target_version, const char *schema_file, json_object* input_jo, int migration_mode);
void rearrange_keys(json_object *target, json_object *schema);

#endif // JSON_UPGRADE_H
 