#ifndef __GPIO_JSON_H__
#define __GPIO_JSON_H__

#include <cjson/cJSON.h>

/**
 * Loads the GPIO definitions from JSON into a cJSON structure.
 *
 * @return cjSON* - The structure with the GPIO info.  Should be freed
 *                  with cJSON_Delete() when done.  NULL is returned
 *                  if there was an error.
 */
cJSON* load_json();

#endif
