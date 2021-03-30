/**
 * Copyright 2016 IBM Corporation
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
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include "mapper.h"

static void quit(int r, void* loop)
{
    sd_event_exit((sd_event*)loop, r);
}

static int wait_main(int argc, char* argv[])
{
    int r;
    sd_bus* conn = NULL;
    sd_event* loop = NULL;
    mapper_async_wait* wait = NULL;
    size_t attempts = 0;
    const size_t max_attempts = 4;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s wait OBJECTPATH...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Mapper waits are typically run early in the boot process, and in some
     * cases the CPU and/or object manager daemon are so busy that the
     * GetObject call may fail with a timeout and cause the event loop to exit.
     * If this happens, retry a few times.  Don't retry on other failures.
     */
    while (1)
    {
        attempts++;

        r = sd_bus_default_system(&conn);
        if (r < 0)
        {
            fprintf(stderr, "Error connecting to system bus: %s\n",
                    strerror(-r));
            goto finish;
        }

        r = sd_event_default(&loop);
        if (r < 0)
        {
            fprintf(stderr, "Error obtaining event loop: %s\n", strerror(-r));

            goto finish;
        }

        r = sd_bus_attach_event(conn, loop, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0)
        {
            fprintf(stderr,
                    "Failed to attach system "
                    "bus to event loop: %s\n",
                    strerror(-r));
            goto finish;
        }

        r = mapper_wait_async(conn, loop, argv + 2, quit, loop, &wait);
        if (r < 0)
        {
            fprintf(stderr, "Error configuring waitlist: %s\n", strerror(-r));
            goto finish;
        }

        r = sd_event_loop(loop);
        if (r < 0)
        {
            fprintf(stderr, "Event loop exited: %s\n", strerror(-r));

            if (-r == ETIMEDOUT || -r == EHOSTUNREACH)
            {
                if (attempts <= max_attempts)
                {
                    fprintf(stderr, "Retrying in 5s\n");
                    sleep(5);
                    sd_event_unref(loop);
                    sd_bus_unref(conn);
                    continue;
                }
                else
                {
                    fprintf(stderr, "Giving up\n");
                }
            }
            else
            {
                goto finish;
            }
        }

        break;
    }

finish:
    exit(r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

static int subtree_main(int argc, char* argv[])
{
    int r = 0;
    int op = 0;
    static const char* token = ":";
    char* tmp = NULL;
    char* namespace = NULL;
    char* interface = NULL;
    sd_bus* conn = NULL;
    sd_event* loop = NULL;
    mapper_async_subtree* subtree = NULL;

    if (argc != 3)
    {
        fprintf(stderr,
                "Usage: %s subtree-remove "
                "NAMESPACE%sINTERFACE\n",
                argv[0], token);
        exit(EXIT_FAILURE);
    }

    op = MAPPER_OP_REMOVE;

    namespace = strtok_r(argv[2], token, &tmp);
    interface = strtok_r(NULL, token, &tmp);
    if ((namespace == NULL) || (interface == NULL))
    {
        fprintf(stderr, "Token '%s' was not found in '%s'\n", token, argv[2]);
        exit(EXIT_FAILURE);
    }

    r = sd_bus_default_system(&conn);
    if (r < 0)
    {
        fprintf(stderr, "Error connecting to system bus: %s\n", strerror(-r));
        goto finish;
    }

    r = sd_event_default(&loop);
    if (r < 0)
    {
        fprintf(stderr, "Error obtaining event loop: %s\n", strerror(-r));
        goto finish;
    }

    r = sd_bus_attach_event(conn, loop, SD_EVENT_PRIORITY_NORMAL);
    if (r < 0)
    {
        fprintf(stderr, "Failed to attach system bus to event loop: %s\n",
                strerror(-r));
        goto finish;
    }

    r = mapper_subtree_async(conn, loop, namespace, interface, quit, loop,
                             &subtree, op);
    if (r < 0)
    {
        fprintf(stderr, "Error configuring subtree list: %s\n", strerror(-r));
        goto finish;
    }

    r = sd_event_loop(loop);
    if (r < 0)
    {
        /* If this function has been called after the interface in   */
        /* question has already been removed, then GetSubTree will   */
        /* fail and it will show up here.  Treat as success instead. */
        if (r == -ENXIO)
        {
            r = 0;
        }
        else
        {
            fprintf(stderr, "Error starting event loop: %d(%s)\n", r,
                    strerror(-r));
            goto finish;
        }
    }

finish:
    exit(r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

/* print out the distinct dbus service name for the input dbus path */
static int get_service_main(int argc, char* argv[])
{
    int r;
    sd_bus* conn = NULL;
    char* service = NULL;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s get-service OBJECTPATH\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    r = sd_bus_default_system(&conn);
    if (r < 0)
    {
        fprintf(stderr, "Error connecting to system bus: %s\n", strerror(-r));
        goto finish;
    }

    r = mapper_get_service(conn, argv[2], &service);
    if (r < 0)
    {
        fprintf(stderr, "Error finding '%s' service: %s\n", argv[2],
                strerror(-r));
        goto finish;
    }

    printf("%s\n", service);

finish:
    exit(r < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    static const char* usage =
        "Usage: %s {COMMAND} ...\n"
        "\nCOMMANDS:\n"
        "  wait           wait for the specified objects to appear on the "
        "DBus\n"
        "  subtree-remove\n"
        "                 wait until the specified interface is not present\n"
        "                 in any of the subtrees of the specified namespace\n"
        "  get-service    return the service identifier for input path\n";

    if (argc < 2)
    {
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[1], "wait"))
        wait_main(argc, argv);
    if (!strcmp(argv[1], "subtree-remove"))
        subtree_main(argc, argv);
    if (!strcmp(argv[1], "get-service"))
        get_service_main(argc, argv);

    fprintf(stderr, usage, argv[0]);
    exit(EXIT_FAILURE);
}
