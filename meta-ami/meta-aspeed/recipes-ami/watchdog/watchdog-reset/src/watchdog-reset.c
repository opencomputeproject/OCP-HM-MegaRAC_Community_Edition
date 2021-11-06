#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define WDT_DEV		"/dev/watchdog"
#define MAGIC_CLOSE	"V"
#define WDT_SELECT	"/sys/module/watchdog/parameters/current_wdt_device"
#define WDT_DEV_INDEX	3

int select_wdt(int devno)
{
	char line[64];
	int len;

	int wdsel = open(WDT_SELECT, O_NONBLOCK | O_WRONLY);
	if (wdsel == -1) {
		printf("Failed to open watchdog select interface\n");
		return -1;
	}

	len = snprintf(line, sizeof(line), "%d\n", devno);
	line[sizeof(line) - 1] = 0;

	if (write(wdsel, line, len) == -1) {
		printf("Failed to select watchdog\n");
		close(wdsel);
		return -1;
	}

	if (close(wdsel) == -1) {
		printf("Failed to close watchdog select interface\n");
	}

	return 0;
}

int stop_wdt()
{
	int wdt;

	if (select_wdt(WDT_DEV_INDEX) < 0) {
		printf("Failed to select watchdog to stop\n");
		return -1;
	}

	wdt = open(WDT_DEV, O_NONBLOCK | O_RDWR);
	if (wdt == -1) {
		printf("Failed to open wachdog timer\n");
		select_wdt(1);
		return -1;
	}

	if (write(wdt, MAGIC_CLOSE, 1) == -1) {
		printf("Failed to write magic close byte!\n");
		printf(strerror(errno));
		close(wdt);
		select_wdt(1);
		return -1;
	}

	int flags = WDIOS_DISABLECARD;
	if (ioctl(wdt, WDIOC_SETOPTIONS, &flags) == -1) {
		printf("Failed to stop watchdog!\n");
		close(wdt);
		select_wdt(1);
		return -1;
	}

	if (close(wdt) == -1) {
		printf("Failed to close watchdog timer\n");
		select_wdt(1);
		return -1;
	}

	if (select_wdt(1) < 0) {
		printf("Failed to restore watchdog\n");
		return -1;
	}

	return 0;
}

pid_t get_pid(char *pname)
{
	char line[1024];
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "pidof %s", pname);
	cmd[sizeof(cmd) - 1] = 0;

	FILE *pipe = popen(cmd, "r");
	fgets(line, sizeof(line), pipe);

	if (line[0] == 0) {
		return 0;
	}

	pid_t pid = strtoul(line, NULL, 10);

	if (pid <= 1) {
		printf("Failed to get pid for process \"%s\"\n", pname);
		return 0;
	}

	return pid;
}

int main(void)
{
	pid_t sentinel = 0;
	for (int i = 0; i < 10; i++) { // try 10 times
		// get the pid
		while (!sentinel) {
			sentinel = get_pid("ipmid");
			sleep(5);
		}
		// make sure it's stable
		sleep(15);
		if (sentinel == get_pid("ipmid")) {
			stop_wdt();
			return 0;
		}
		// unstable, reset and try again
		sentinel = 0;
	}

	return -1;
}
