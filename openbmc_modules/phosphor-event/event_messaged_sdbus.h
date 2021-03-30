#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	int start_event_monitor(void);
	int build_bus(event_manager *em);
	int send_log_to_dbus(event_manager *em, const uint16_t logid, const char* association);
	void cleanup_event_monitor(void);
#ifdef __cplusplus
}
#endif
