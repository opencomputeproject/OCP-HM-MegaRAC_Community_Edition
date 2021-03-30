#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

static void ShowUsage ( void )
{
    printf("Bios Code Test Tool\n");
    printf("Usage : bioscode <snoop_channel> \n");
    printf("\t<snoop_channel>  specifies the SNOOP channel used to SNOOP data reading test\n");
    printf("\n");
}

static int process_arguments( int argc, char** argv, unsigned int* p_snoop_channel)
{
    int i = 1;

    if (argc < 2)
    {
        ShowUsage();
        return -1;
    }
    else
    {
        *p_snoop_channel = (unsigned int)strtol( argv[i++], NULL, 10);
    }

    return 0;
}

int main( int argc, char** argv )
{
    char buf[1024] = {0}, path[1024] = {0};
    int fd, i, n, ret ;
    short revents;
    struct pollfd pfds;
    unsigned int snoop_channel = 0;

    ret = process_arguments (argc, argv, &snoop_channel);
    if (ret != 0)
    return -1;

    snprintf(path, sizeof(path), "/dev/aspeed-lpc-snoop%d", snoop_channel);

    /* O_NONBLOCK is required or else the open blocks
    * until the other side of the pipe opens. */
    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        printf("open failed, No device file %s \n", path);
        exit(EXIT_FAILURE);
    }

    // POLLIN - receive signal when data is available to read.
    pfds.fd = fd;
    pfds.events = POLLIN;

    while (1)
    {
        // Wait infinite time for available data in /dev/aspeed-lpc-snoop
        i = poll(&pfds,(nfds_t ) 1, -1);
        if (i == -1)
        {
            printf("poll failed \n");
            exit(EXIT_FAILURE);
        }

        // Received events.
        revents = pfds.revents;
        if (revents & POLLIN)
        {
            n = read(pfds.fd, buf, sizeof(buf));
            for (i = 0; i < n; i++)
            {
                printf("current post code = %x \n ", buf[i] );
            }
        }
    }

    return 0;
}

