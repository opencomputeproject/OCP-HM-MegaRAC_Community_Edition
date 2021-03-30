#include <cerrno>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <functional>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/signal.hpp>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void reader(const char* fifo, sdeventplus::source::IO& source, int fd, uint32_t)
{
    char buf[4096];
    ssize_t r = read(fd, buf, sizeof(buf));
    if (r == 0)
    {
        int newfd = open(fifo, O_NONBLOCK | O_RDONLY);
        if (newfd < 0)
        {
            fprintf(stderr, "Failed to open %s: %s\n", fifo, strerror(errno));
            source.get_event().exit(1);
            return;
        }
        source.set_fd(newfd);
        if (close(fd))
        {
            fprintf(stderr, "Failed to close fd\n");
            source.get_event().exit(1);
            return;
        }
        return;
    }
    if (r < 0)
    {
        fprintf(stderr, "Reader error: %s\n", strerror(errno));
        source.get_event().exit(1);
        return;
    }
    printf("%.*s", static_cast<int>(r), buf);
}

void remover(const char* fifo, sdeventplus::source::EventBase& source)
{
    int r = unlink(fifo);
    if (r)
    {
        fprintf(stderr, "Failed to remove fifo %s: %s\n", fifo,
                strerror(errno));
        source.get_event().exit(1);
    }
}

void clean_exit(sdeventplus::source::Signal& source,
                const struct signalfd_siginfo*)
{
    source.get_event().exit(0);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [named pipe to create]\n", argv[0]);
        return 1;
    }
    const char* fifo = argv[1];

    // Block all signals before changing system state so we guarantee our clean
    // up routines are in place
    sigset_t signals;
    if (sigfillset(&signals))
    {
        fprintf(stderr, "Failed to populate signals: %s\n", strerror(errno));
        return 1;
    }
    if (sigprocmask(SIG_BLOCK, &signals, nullptr))
    {
        fprintf(stderr, "Failed to mask signals: %s\n", strerror(errno));
        return 1;
    }

    if (mkfifo(fifo, 0622))
    {
        fprintf(stderr, "Failed to mkfifo %s: %s\n", fifo, strerror(errno));
        return 1;
    }

    int fd = open(fifo, O_NONBLOCK | O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Failed to open %s: %s\n", fifo, strerror(errno));
        return 1;
    }

    try
    {
        sdeventplus::Event event = sdeventplus::Event::get_default();
        sdeventplus::source::Exit remover_source(
            event, std::bind(remover, fifo, std::placeholders::_1));
        sdeventplus::source::Signal sigint(event, SIGINT, clean_exit);
        sdeventplus::source::IO reader_source(
            event, fd, EPOLLIN,
            std::bind(reader, fifo, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));
        return event.loop();
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }
}
