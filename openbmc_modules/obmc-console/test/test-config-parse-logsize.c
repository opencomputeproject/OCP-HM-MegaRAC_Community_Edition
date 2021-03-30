#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// Bypass compilation error due to -DSYSCONFDIR not provided
#define SYSCONFDIR

#include "config.c"

struct test_parse_size_unit {
	const char	*test_str;
	size_t		expected_size;
	int		expected_rc;
};

void test_config_parse_logsize(void)
{
	const struct test_parse_size_unit test_data[] = {
		{"0",		0,		-1},
		{"1",		1,		0},
		{"4k",		4*1024,		0},
		{"6M",		(6ul << 20),	0},
		{"4095M",	(4095ul << 20),	0},
		{"2G",		(2ul << 30),	0},
		{"8M\n",	(8ul << 20),	0},	/* Suffix ignored */
		{" 10k",	10*1024,	0},	/* Leading spaces trimmed */
		{"10k ",	10*1024,	0},	/* Trailing spaces trimmed */
		{"\r\t10k \r\t",10*1024,	0},	/* Spaces trimmed */
		{" 10 kB ",	10*1024,	0},	/* Spaces trimmed */
		{"11G",		0,		-1},	/* Overflow */
		{"4294967296",	0,		-1},	/* Overflow */
		{"4096M",	0,		-1},	/* Overflow */
		{"65535G",	0,		-1},	/* Overflow */
		{"xyz",		0,		-1},	/* Invalid */
		{"000",		0,		-1},	/* Invalid */
		{"0.1",		0,		-1},	/* Invalid */
		{"9T",		0,		-1},	/* Invalid suffix */
	};
	const size_t num_tests = sizeof(test_data) /
		sizeof(struct test_parse_size_unit);
	size_t size;
	int i, rc;

	for (i = 0; i < num_tests; i++) {
		rc = config_parse_logsize(test_data[i].test_str, &size);

		if ((rc == -1 && rc != test_data[i].expected_rc) ||
		    (rc == 0 && test_data[i].expected_size != size)) {
			warn("[%d] Str %s expected size %lu rc %d,"
                             " got size %lu rc %d\n",
			     i,
			     test_data[i].test_str,
			     test_data[i].expected_size,
			     test_data[i].expected_rc,
			     size,
			     rc);
		}
		assert(rc == test_data[i].expected_rc);
		if (rc == 0)
			assert(size == test_data[i].expected_size);
	}
}

int main(void)
{
	test_config_parse_logsize();
	return EXIT_SUCCESS;
}
