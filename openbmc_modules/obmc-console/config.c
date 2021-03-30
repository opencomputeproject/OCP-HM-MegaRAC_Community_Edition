/**
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

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h> /* for speed_t */
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include "console-server.h"

static const char *config_default_filename = SYSCONFDIR "/obmc-console.conf";

struct config_item {
	char			*name;
	char			*value;
	struct config_item	*next;
};

struct config {
	struct config_item	*items;
};

const char *config_get_value(struct config *config, const char *name)
{
	struct config_item *item;

	for (item = config->items; item; item = item->next)
		if (!strcasecmp(item->name, name))
			return item->value;

	return NULL;
}

static void config_parse(struct config *config, char *buf)
{
	struct config_item *item;
	char *name, *value;
	char *p, *line;
	int rc;

	for (p = NULL, line = strtok_r(buf, "\n", &p); line;
			line = strtok_r(NULL, "\n", &p)) {

		/* trim leading space */
		for (;*line == ' ' || *line == '\t'; line++)
			;

		/* skip comments */
		if (*line == '#')
			continue;

		name = value = NULL;

		rc = sscanf(line, "%m[^ =] = %ms ", &name, &value);
		if (rc != 2 || !strlen(name) || !strlen(value)) {
			free(name);
			free(value);
			continue;
		}

		/* create a new item and add to our list */
		item = malloc(sizeof(*item));
		item->name = name;
		item->value = value;
		item->next = config->items;
		config->items = item;
	}
}

static struct config *config_init_fd(int fd, const char *filename)
{
	struct config *config;
	size_t size, len;
	char *buf;
	int rc;

	size = 4096;
	len = 0;
	buf = malloc(size + 1);
	config = NULL;

	for (;;) {
		rc = read(fd, buf + len, size - len);
		if (rc < 0) {
			warn("Can't read from configuration file %s", filename);
			goto out_free;

		} else if (!rc) {
			break;
		}
		len += rc;
		if (len == size) {
			size <<= 1;
			buf = realloc(buf, size + 1);
		}

	}
	buf[len] = '\0';

	config = malloc(sizeof(*config));
	config->items = NULL;

	config_parse(config, buf);

out_free:
	free(buf);
	return config;
}

struct config *config_init(const char *filename)
{
	struct config *config;
	int fd;

	if (!filename)
		filename = config_default_filename;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		warn("Can't open configuration file %s", filename);
		return NULL;
	}

	config = config_init_fd(fd, filename);

	close(fd);

	return config;
}

void config_fini(struct config *config)
{
	struct config_item *item, *next;

	for (item = config->items; item; item = next) {
		next = item->next;
		free(item->name);
		free(item->value);
		free(item);
	}

	free(config);
}

struct terminal_speed_name {
	speed_t		speed;
	uint32_t	baud;
	const char	*name;
};

#define TERM_SPEED(x) { B##x, x, #x}

static const struct terminal_speed_name terminal_speeds[] = {
	TERM_SPEED(50),
	TERM_SPEED(75),
	TERM_SPEED(110),
	TERM_SPEED(134),
	TERM_SPEED(150),
	TERM_SPEED(200),
	TERM_SPEED(300),
	TERM_SPEED(600),
	TERM_SPEED(1200),
	TERM_SPEED(1800),
	TERM_SPEED(2400),
	TERM_SPEED(4800),
	TERM_SPEED(9600),
	TERM_SPEED(19200),
	TERM_SPEED(38400),
	TERM_SPEED(57600),
	TERM_SPEED(115200),
	TERM_SPEED(230400),
	TERM_SPEED(460800),
	TERM_SPEED(500000),
	TERM_SPEED(576000),
	TERM_SPEED(921600),
	TERM_SPEED(1000000),
	TERM_SPEED(1152000),
	TERM_SPEED(1500000),
	TERM_SPEED(2000000),
	TERM_SPEED(2500000),
	TERM_SPEED(3000000),
	TERM_SPEED(3500000),
	TERM_SPEED(4000000),
};

int config_parse_baud(speed_t *speed, const char *baud_string)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(terminal_speeds); i++) {
		if (strcmp(baud_string, terminal_speeds[i].name) == 0) {
			*speed = terminal_speeds[i].speed;
			return 0;
		}
	}
	return -1;
}

uint32_t parse_baud_to_int(speed_t speed)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(terminal_speeds); i++) {
		if (terminal_speeds[i].speed == speed) {
			return terminal_speeds[i].baud;
		}
	}
	return 0;
}

speed_t parse_int_to_baud(uint32_t baud)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(terminal_speeds); i++) {
		if (terminal_speeds[i].baud == baud) {
			return terminal_speeds[i].speed;
		}
	}
	return 0;
}

int config_parse_logsize(const char *size_str, size_t *size)
{
	struct size_suffix_shift {
		/* Left shiftwidth corresponding to the suffix. */
		size_t	shiftwidth;
		int	unit;
	};

	const struct size_suffix_shift suffixes[] = {
		{ 10, 'k' },
		{ 20, 'M' },
		{ 30, 'G' },
	};
	const size_t num_suffixes = sizeof(suffixes) /
				    sizeof(struct size_suffix_shift);
	size_t logsize;
	char *suffix;
	size_t i;

	if (!size_str)
		return -1;

	logsize = strtoul(size_str, &suffix, 0);
	if (logsize == 0 || logsize >= UINT32_MAX || suffix == size_str)
		return -1;

	/* Ignore spaces between number and suffix */
	while (*suffix && isspace(*suffix))
		suffix++;

	for (i = 0; i < num_suffixes; i++) {
		if (*suffix == suffixes[i].unit) {
			/*
			 * If logsize overflows, probably something was wrong.
			 * Return instead of clamping to an arbitrary value.
			 */
			if (logsize > (UINT32_MAX >> suffixes[i].shiftwidth))
				return -1;

			logsize <<= suffixes[i].shiftwidth;
			suffix++;
			break;
		}
	}

	/* Allow suffix like 'kB' */
	while (*suffix && (tolower(*suffix) == 'b' || isspace(*suffix)))
		suffix++;

	if (*suffix) {
		warn("Invalid suffix!");
		return -1;
	}

	*size = logsize;
	return 0;
}

#ifdef CONFIG_TEST
int main(void)
{
	struct config_item *item;
	struct config *config;

	config = config_init_fd(STDIN_FILENO, "<stdin>");
	if (!config)
		return EXIT_FAILURE;

	for (item = config->items; item; item = item->next)
		printf("%s: %s\n", item->name, item->value);

	config_fini(config);

	return EXIT_SUCCESS;

}
#endif
