#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum mapper_operation
{
    MAPPER_OP_REMOVE = 1
} mapper_operation;
typedef struct mapper_async_wait mapper_async_wait;
typedef struct mapper_async_subtree mapper_async_subtree;
void mapper_wait_async_free(mapper_async_wait*);
void mapper_subtree_async_free(mapper_async_subtree*);

int mapper_wait_async(sd_bus*, sd_event*, char*[], void (*)(int, void*), void*,
                      mapper_async_wait**);
int mapper_subtree_async(sd_bus*, sd_event*, char*, char*, void (*)(int, void*),
                         void*, mapper_async_subtree**, int);
int mapper_get_service(sd_bus* conn, const char* obj, char** service);
int mapper_get_object(sd_bus* conn, const char* obj, sd_bus_message** reply);
#ifdef __cplusplus
}
#endif
