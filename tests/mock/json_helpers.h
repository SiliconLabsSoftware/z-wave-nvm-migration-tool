
/* © 2019 Silicon Laboratories Inc. */
#ifndef _JSON_HELPERS_H
#define _JSON_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <json.h>

struct json_object
{
	enum json_type o_type;
	uint32_t _ref_count;
	json_object_to_json_string_fn *_to_json_string;
	struct printbuf *_pb;
	json_object_delete_fn *_user_delete;
	void *_userdata;
};
#define JSON_REQUIRED true
#define JSON_OPTIONAL false

void set_json_parse_error_flag(bool parse_error_detected);
bool json_parse_error_detected(void);

// Generating JSON
void json_add_int(json_object* jo, const char *key, int32_t val);
void json_add_bool(json_object* jo, const char *key, bool val);
void json_add_string(json_object* jo, const char *key, const char *val);
void json_add_nodemask(json_object* jo, const char *key, const uint8_t *nodemask_val);
void json_add_byte_array(json_object* jo, const char *key, const uint8_t *array, uint32_t len);

json_object* home_id_to_json(const uint8_t * home_id_buf);
json_object* version_to_json(uint32_t version);
json_object* nodemask_to_json(const uint8_t *nodemask);

// Parsing JSON
bool json_get_object_error_check(json_object* obj,
                                 const char *key,
                                 json_object **value,
                                 enum json_type expected_type,
                                 bool is_required);
int32_t json_get_int(json_object* parent, const char *key, int32_t default_val, bool is_required);
bool json_get_bool(json_object* parent, const char *key, bool default_val, bool is_required);
const char * json_get_string(json_object* parent, const char *key, const char *default_val, bool is_required);
bool json_get_nodemask(json_object* parent, const char *key, uint8_t *nodemask, bool is_required);
uint8_t json_get_bytearray(json_object* parent, const char *key, uint8_t *array, uint32_t length, bool is_required);
uint8_t json_get_intarray(json_object* parent, const char *key, int *array, uint32_t length, bool is_required);
uint32_t json_get_home_id(json_object* parent, const char *key, uint8_t *home_id_buf, uint32_t default_val, bool is_required);
json_object* json_get_object(json_object* parent, const char *key, json_object* default_val, bool is_required);

const char * json_get_object_path(const json_object* obj);
void json_register_root(json_object* obj);

json_object* json_object_get_from_path(json_object* top, const char* path);

#endif // _JSON_HELPERS_H