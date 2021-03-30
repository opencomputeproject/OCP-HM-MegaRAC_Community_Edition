
#include <dirent.h>
#include <time.h>

#ifdef __cplusplus
	#include <cstdint>
	#include <string>

	using namespace std;
#else
	#include <stdint.h>
#endif

#ifdef __cplusplus
	struct event_record_t {
#else
	typedef struct _event_record_t {
#endif
		char *message;
		char *severity;
		char *association;
		char *reportedby;
		uint8_t *p;
		size_t n;

		// These get filled in for you
		time_t  timestamp;        
		int16_t logid;
#ifdef __cplusplus
	};

#else
	} event_record_t;
#endif


#ifdef __cplusplus

class event_manager {
	uint16_t latestid;
	string   eventpath;
	DIR      *dirp;
	uint16_t logcount;
	uint16_t maxlogs;
	size_t   maxsize;
	size_t   currentsize;

public:
	event_manager(string path, size_t reqmaxsize, uint16_t reqmaxlogs);
	~event_manager();

	uint16_t next_log(void);
	void     next_log_refresh(void);

	uint16_t latest_log_id(void);
	uint16_t log_count(void);
	size_t   get_managed_size(void);

	int      open(uint16_t logid, event_record_t **rec); // must call close
	void     close(event_record_t *rec);

	uint16_t create(event_record_t *rec);
	int      remove(uint16_t logid);

private:
	bool is_file_a_log(string str);
	uint16_t create_log_event(event_record_t *rec);
	uint16_t new_log_id(void);
	bool     is_logid_a_log(uint16_t logid);
};
#else
typedef struct event_manager event_manager;
#endif

#ifdef __cplusplus
extern "C"  {
#endif
uint16_t message_create_new_log_event(event_manager *em, event_record_t *rec);
int      message_load_log(event_manager *em, uint16_t logid, event_record_t **rec);
void     message_free_log(event_manager *em, event_record_t *rec);
int      message_delete_log(event_manager *em, uint16_t logid);
void     message_refresh_events(event_manager *em);
uint16_t message_next_event(event_manager *em);
#ifdef __cplusplus
}
#endif


