/**
 * Copyright © 2016 Google Inc.
 * Copyright © 2016 IBM Corporation
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

#include "gpio_configs.h"
#include "gpio_json.h"
#include <cjson/cJSON.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>


/**
 * Loads the GPIO information into the gpios->power_gpio structure
 * from the JSON.
 *
 * @param gpios - the structure where GpioConfigs.power_gpio will
 *                be filled in.
 * @param gpio_configs - cJSON pointer to the GPIO JSON
 */
void read_power_gpios(GpioConfigs* gpios, const cJSON* gpio_configs)
{
	size_t i = 0;

	const cJSON* power_config = cJSON_GetObjectItem(
			gpio_configs, "power_config");
	g_assert(power_config != NULL);

	/* PGOOD - required */

	const cJSON* pgood = cJSON_GetObjectItem(power_config, "power_good_in");
	g_assert(pgood != NULL);

	gpios->power_gpio.power_good_in.name = g_strdup(pgood->valuestring);

	g_print("Power GPIO power good input: %s\n",
			gpios->power_gpio.power_good_in.name);

	/* Latch out - optional */

	const cJSON* latch = cJSON_GetObjectItem(power_config, "latch_out");
	if (latch != NULL)
	{
		gpios->power_gpio.latch_out.name = g_strdup(latch->valuestring);
		g_print("Power GPIO latch output: %s\n",
				gpios->power_gpio.latch_out.name);
	}
	else
	{
		//Must be NULL if not there
		gpios->power_gpio.latch_out.name = NULL;
	}

	/* Power Up Outs - required */

	const cJSON* power_up_outs = cJSON_GetObjectItem(
			power_config, "power_up_outs");
	g_assert(power_up_outs != NULL);

	gpios->power_gpio.num_power_up_outs = cJSON_GetArraySize(power_up_outs);
	g_print("Power GPIO %zu power_up outputs\n",
			gpios->power_gpio.num_power_up_outs);

	if (gpios->power_gpio.num_power_up_outs != 0)
	{
		gpios->power_gpio.power_up_outs =
			g_malloc0_n(gpios->power_gpio.num_power_up_outs, sizeof(GPIO));
		gpios->power_gpio.power_up_pols =
			g_malloc0_n(gpios->power_gpio.num_power_up_outs, sizeof(gboolean));

		const cJSON* power_out;
		cJSON_ArrayForEach(power_out, power_up_outs)
		{
			cJSON* name = cJSON_GetObjectItem(power_out, "name");
			g_assert(name != NULL);
			gpios->power_gpio.power_up_outs[i].name =
				g_strdup(name->valuestring);

			const cJSON* polarity = cJSON_GetObjectItem(power_out, "polarity");
			g_assert(polarity != NULL);
			gpios->power_gpio.power_up_pols[i] = polarity->valueint;

			g_print("Power GPIO power_up[%d] = %s active %s\n",
					i, gpios->power_gpio.power_up_outs[i].name,
					gpios->power_gpio.power_up_pols[i] ? "HIGH" : "LOW");
			i++;
		}
	}

	/* Resets - optional */

	const cJSON* reset_outs = cJSON_GetObjectItem(power_config, "reset_outs");
	gpios->power_gpio.num_reset_outs = cJSON_GetArraySize(reset_outs);

	g_print("Power GPIO %zu reset outputs\n",
			gpios->power_gpio.num_reset_outs);

	if (gpios->power_gpio.num_reset_outs != 0)
	{
		gpios->power_gpio.reset_outs =
			g_malloc0_n(gpios->power_gpio.num_reset_outs, sizeof(GPIO));
		gpios->power_gpio.reset_pols =
			g_malloc0_n(gpios->power_gpio.num_reset_outs, sizeof(gboolean));

		i = 0;
		const cJSON* reset_out;
		cJSON_ArrayForEach(reset_out, reset_outs)
		{
			cJSON* name = cJSON_GetObjectItem(reset_out, "name");
			g_assert(name != NULL);
			gpios->power_gpio.reset_outs[i].name = g_strdup(name->valuestring);

			const cJSON* polarity = cJSON_GetObjectItem(reset_out, "polarity");
			g_assert(polarity != NULL);
			gpios->power_gpio.reset_pols[i] = polarity->valueint;

			g_print("Power GPIO reset[%d] = %s active %s\n", i,
					gpios->power_gpio.reset_outs[i].name,
					gpios->power_gpio.reset_pols[i] ? "HIGH" : "LOW");
			i++;
		}
	}

	/* PCI Resets - optional */

	const cJSON* pci_reset_outs = cJSON_GetObjectItem(
			power_config, "pci_reset_outs");

	gpios->power_gpio.num_pci_reset_outs =
		cJSON_GetArraySize(pci_reset_outs);

	g_print("Power GPIO %zd pci reset outputs\n",
			gpios->power_gpio.num_pci_reset_outs);

	if (gpios->power_gpio.num_pci_reset_outs != 0)
	{
		gpios->power_gpio.pci_reset_outs =
			g_malloc0_n(gpios->power_gpio.num_pci_reset_outs, sizeof(GPIO));
		gpios->power_gpio.pci_reset_pols =
			g_malloc0_n(gpios->power_gpio.num_pci_reset_outs, sizeof(gboolean));
		gpios->power_gpio.pci_reset_holds =
			g_malloc0_n(gpios->power_gpio.num_pci_reset_outs, sizeof(gboolean));

		i = 0;
		const cJSON* pci_reset_out;
		cJSON_ArrayForEach(pci_reset_out, pci_reset_outs)
		{
			cJSON* name = cJSON_GetObjectItem(pci_reset_out, "name");
			g_assert(name != NULL);
            gpios->power_gpio.pci_reset_outs[i].name =
                g_strdup(name->valuestring);

			const cJSON* polarity = cJSON_GetObjectItem(
                    pci_reset_out, "polarity");
			g_assert(polarity != NULL);
			gpios->power_gpio.pci_reset_pols[i] = polarity->valueint;

			const cJSON* hold = cJSON_GetObjectItem(pci_reset_out, "hold");
			g_assert(hold != NULL);
			gpios->power_gpio.pci_reset_holds[i] = polarity->valueint;

			g_print("Power GPIO pci reset[%d] = %s active %s, hold %s\n", i,
					gpios->power_gpio.pci_reset_outs[i].name,
					gpios->power_gpio.pci_reset_pols[i] ? "HIGH" : "LOW",
					gpios->power_gpio.pci_reset_holds[i] ? "Yes" : "No");
			i++;
		}
	}
}

gboolean read_gpios(GpioConfigs *gpios)
{
	cJSON* json = load_json();
	if (json == NULL)
	{
		return FALSE;
	}

	const cJSON* configs = cJSON_GetObjectItem(json, "gpio_configs");
	g_assert(configs != NULL);

	read_power_gpios(gpios, configs);

	cJSON_Delete(json);
	return TRUE;
}

void free_gpios(GpioConfigs *gpios) {
	int i;
	g_free(gpios->power_gpio.latch_out.name);
	g_free(gpios->power_gpio.power_good_in.name);
	for(i = 0; i < gpios->power_gpio.num_power_up_outs; i++) {
		g_free(gpios->power_gpio.power_up_outs[i].name);
	}
	g_free(gpios->power_gpio.power_up_outs);
	g_free(gpios->power_gpio.power_up_pols);
	for(i = 0; i < gpios->power_gpio.num_reset_outs; i++) {
		g_free(gpios->power_gpio.reset_outs[i].name);
	}
	g_free(gpios->power_gpio.reset_outs);
	g_free(gpios->power_gpio.reset_pols);
	for(i = 0; i < gpios->power_gpio.num_pci_reset_outs; i++) {
		g_free(gpios->power_gpio.pci_reset_outs[i].name);
	}
	g_free(gpios->power_gpio.pci_reset_outs);
	g_free(gpios->power_gpio.pci_reset_pols);
	g_free(gpios->power_gpio.pci_reset_holds);
}
