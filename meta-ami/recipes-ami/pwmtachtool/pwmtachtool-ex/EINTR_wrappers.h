
/*File:EINTR_wrappers.h 
* 
* Wrapper functions header file.
*
* Copyright (C) <2019>  <American Megatrends International LLC>
*
*/

#ifndef EINTR_WRAPPERS_H__
#define EINTR_WRAPPERS_H__

#if defined(__linux__)
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _SYS_PC_H
#include <sys/ipc.h>
#endif

#ifndef _SYS_SEM_H
#include <sys/sem.h>
#endif

#ifndef _SYS_EPOLL_H
#include <sys/epoll.h>
#endif

#ifndef _SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifndef _SIGNAL_H
#include <signal.h>
#endif

#ifndef _TIME_H
#include <time.h>
#endif

#ifndef _POLL_H
#include <poll.h>
#endif

#ifndef _WAIT_H
#include <wait.h>
#endif

#ifndef _UNISTD_H
#include <unistd.h>
#endif

int  sigwrap_semop(int semid, struct sembuf *sops, size_t nsops);
int  sigwrap_semtimedop(int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout);
int  sigwrap_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
int  sigwrap_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask);
int  sigwrap_sigwaitinfo(const sigset_t *set, siginfo_t *info);
int  sigwrap_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
int  sigwrap_nanosleep(const struct timespec *req, struct timespec *rem);
int  sigwrap_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);
int  sigwrap_usleep(useconds_t usec);
int  sigwrap_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int  sigwrap_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask);
int  sigwrap_select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
int  sigwrap_pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);
int  sigwrap_msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
int  sigwrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int  sigwrap_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int  sigwrap_accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
int  sigwrap_close(int hFile);
int sigwrap_open_mode(const char *pathname, int flags, mode_t mode);
int  sigwrap_open(const char *pathname, int flags);


pid_t sigwrap_wait(int *status);
pid_t sigwrap_waitpid(pid_t pid, int *status, int options);
int   sigwrap_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);


ssize_t sigwrap_msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
ssize_t sigwrap_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t sigwrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t sigwrap_sendsendmsg(int sockfd, const struct msghdr *msg, int flags);

ssize_t sigwrap_read(int fd, void *buf, size_t count);
// EINTR wrapper for the standard read() function. Waits until ALL requested data is available. Use the non-blocking version (sigwrap_read)
// for sockets that are set to non-blocking mode or when partial data is okay
// Although the description for the read() function describes it differently, it seems possible that the original function may already return
// even though partial data has already been read. This implementation makes sure that all requested data have been read.
// See the comment in the signal description https://linux.die.net/man/7/signal
//* read(2), readv(2), write(2), writev(2), and ioctl(2) calls on "slow" devices. 
//* A "slow" device is one where the I/O call may block for an indefinite time, for example, a terminal, pipe, or socket. 
//* (A disk is not a slow device according to this definition.) If an I/O call on a slow device has already transferred
//* some data by the time it is interrupted by a signal handler, then the call will return a success status (normally, the number of bytes transferred). 
ssize_t sigwrap_blocking_read(int hFile, void *pData, size_t RdLen);

ssize_t sigwrap_readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t sigwrap_write(int fd, const void *buf, size_t count);
// EINTR wrapper for the standard write() function. Waits until ALL data is written! Use the non-blocking version (sigwrap_write)
// for sockets that are set to non-blocking mode, or when it is OK to write only partial data.
// Although the description for the write() function describes it differently, it seems possible that the original function may already return
// even though partial data has already been written. This implementation makes sure that all requested data have been written.
// See the comment in the signal description https://linux.die.net/man/7/signal
//* read(2), readv(2), write(2), writev(2), and ioctl(2) calls on "slow" devices. 
//* A "slow" device is one where the I/O call may block for an indefinite time, for example, a terminal, pipe, or socket. 
//* (A disk is not a slow device according to this definition.) If an I/O call on a slow device has already transferred
//* some data by the time it is interrupted by a signal handler, then the call will return a success status (normally, the number of bytes transferred). 
ssize_t sigwrap_blocking_write(int hFile, const void *pData, ssize_t WrtLen);

ssize_t sigwrap_writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t sigwrap_recv(int sockfd, void *buf, size_t len, int flags);

ssize_t sigwrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

ssize_t sigwrap_recvmsg(int sockfd, struct msghdr *msg, int flags);

int sigwrap_flock(int fd, int operation);


#endif
#endif
