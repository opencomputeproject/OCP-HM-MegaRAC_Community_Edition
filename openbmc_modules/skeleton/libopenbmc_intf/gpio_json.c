/**
 * Copyright © 2018 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include "gpio_json.h"

#define GPIO_FILE "/etc/default/obmc/gpio/gpio_defs.json"

cJSON* load_json()
{
	FILE* fd = fopen(GPIO_FILE, "r");
	if (!fd)
	{
		fprintf(stderr, "Unable to open GPIO JSON file %s\n", GPIO_FILE);
		return NULL;
	}

	fseek(fd, 0, SEEK_END);
	long size = ftell(fd);
	rewind(fd);

	char* data = malloc(size + 1);

	size_t rc = fread(data, 1, size, fd);
	fclose(fd);
	if (rc != size)
	{
		free(data);
		fprintf(stderr, "Only read %d out of %ld bytes of GPIO file %s\n",
				rc, size, GPIO_FILE);
		return NULL;
	}

	data[size] = '\0';
	cJSON* json = cJSON_Parse(data);
	free(data);

	if (json == NULL)
	{
		fprintf(stderr, "Failed parsing GPIO file %s\n", GPIO_FILE);

		const char* error_loc = cJSON_GetErrorPtr();
		if (error_loc != NULL)
		{
			fprintf(stderr, "JSON error at %s\n", error_loc);
		}
	}
	return json;
}
