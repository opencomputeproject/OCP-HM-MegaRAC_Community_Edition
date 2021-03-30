
/*
* File: EINTR_wrappers.c
*
* This file implements the wrapper functions for some of the System APIs
*
* Copyright (C) <2019>  <American Megatrends International LLC>
*
*/

#include "EINTR_wrappers.h"
#if defined(__linux__)
#include <sys/msg.h>
#include <sys/file.h>
#endif
#include <errno.h>
#include <unistd.h>

static const int OneSecondasNS = 1000000000;

#ifndef bool
typedef int bool;
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

typedef struct
{
	bool OnePoll;
	struct timespec EndTime, Timeout;
} SIGWRAP_TIMEOUT;

static void sigwrap_InitTimeout(SIGWRAP_TIMEOUT *pDst, const struct timespec *timeout)
{
	pDst->Timeout = *timeout;

	if ((timeout->tv_sec == 0) && (timeout->tv_nsec == 0))                  // If both value are zero than only a single poll is requested!
	{
		pDst->OnePoll = 1;
		return;
	}

	pDst->OnePoll = 0;

	struct timespec Now;

	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &Now);                         // CLOCK_MONOTONIC_RAW is not affected by NTP etc.

	pDst->EndTime.tv_sec = Now.tv_sec + pDst->Timeout.tv_sec;               // Check necessary in 2038 due to signed integer variables
	pDst->EndTime.tv_nsec = Now.tv_nsec + pDst->Timeout.tv_nsec;

	if (pDst->EndTime.tv_nsec >= OneSecondasNS)
	{
		pDst->EndTime.tv_sec += (pDst->EndTime.tv_nsec / OneSecondasNS);
		pDst->EndTime.tv_nsec = (pDst->EndTime.tv_nsec % OneSecondasNS);
	}
}


static bool sigwrap_CheckTimeout(SIGWRAP_TIMEOUT *pTo)
{
	if (pTo->OnePoll == TRUE) // Make sure, that in the case that a single poll is requested at least one call is not terminated with EINTR
		return FALSE;

	struct timespec Now;

	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &Now);

	if (Now.tv_sec > pTo->EndTime.tv_sec) // Can become a problem already in 2038 due to signed integer variables
		return TRUE;

	pTo->Timeout.tv_nsec = pTo->EndTime.tv_nsec - Now.tv_nsec;
	pTo->Timeout.tv_sec = pTo->EndTime.tv_sec - Now.tv_sec;

	if (pTo->Timeout.tv_sec == 0)
	{
		if (pTo->Timeout.tv_nsec <= 0)
			return TRUE;
	}
	else if (pTo->Timeout.tv_nsec < 0)
	{
		pTo->Timeout.tv_nsec += OneSecondasNS;
		pTo->Timeout.tv_sec--;
	}

	return FALSE;
}



int sigwrap_semop(int semid, struct sembuf *sops, size_t nsops)
{
	while (1)
	{
		if (semop(semid, sops, nsops) == 0)
			return 0;

		if (errno != EINTR)
			return -1;
	}
}

int sigwrap_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
	SIGWRAP_TIMEOUT To;

	if (timeout != -1)
	{
		struct timespec Timeout;

		Timeout.tv_sec = timeout / 1000;
		Timeout.tv_nsec = (timeout % 1000) * 1000000; // Convert msec to nsec

		sigwrap_InitTimeout(&To, &Timeout);
	}

	while (1)
	{
		int Result = epoll_wait(epfd, events, maxevents, timeout);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;

		if (timeout == -1)
			continue;

		if (sigwrap_CheckTimeout(&To))
			return 0;

		timeout = To.Timeout.tv_sec * 1000 + To.Timeout.tv_nsec / 1000000;
	}
}


int sigwrap_epoll_pwait(int epfd, struct epoll_event *events, int maxevents, int timeout, const sigset_t *sigmask)
{
	SIGWRAP_TIMEOUT To;

	if (timeout != -1)
	{
		struct timespec Timeout;

		Timeout.tv_sec = timeout / 1000;
		Timeout.tv_nsec = (timeout % 1000) * 1000000; // Convert msec to nsec

		sigwrap_InitTimeout(&To, &Timeout);
	}

	while (1)
	{
		int Result = epoll_pwait(epfd, events, maxevents, timeout, sigmask);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;

		if (timeout == -1)
			continue;

		if (sigwrap_CheckTimeout(&To))
			return 0;

		timeout = To.Timeout.tv_sec * 1000 + To.Timeout.tv_nsec / 1000000;
	}
}


int sigwrap_sigwaitinfo(const sigset_t *set, siginfo_t *info)
{
	while (1)
	{
		int Result = sigwaitinfo(set, info);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


int sigwrap_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout)
{
	SIGWRAP_TIMEOUT To;

	sigwrap_InitTimeout(&To, timeout);

	while (1)
	{
		int Result = sigtimedwait(set, info, &To.Timeout);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;

		if (sigwrap_CheckTimeout(&To))
			return 0;
	}
}


int sigwrap_nanosleep(const struct timespec *req, struct timespec *rem)
{
	struct timespec Wait, Remain;

	if (!rem)
		rem = &Remain;

	Wait = *req;

	while (1)
	{
		if (nanosleep(&Wait, rem) == 0)
			return 0;

		if (errno != EINTR)
			return -1;

		Wait = *rem;
	}
}


int sigwrap_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain)
{
	struct timespec Wait, Remain;

	if (!remain)
		remain = &Remain;

	Wait = *request;

	while (1)
	{
		int Result = clock_nanosleep(clock_id, flags, &Wait, remain);

		if (Result == 0)
			return Result;

		if (Result != EINTR)
			return Result;

		if (flags != TIMER_ABSTIME)
			Wait = *remain;
	}
}


int sigwrap_usleep(useconds_t usec)
{
	SIGWRAP_TIMEOUT To;

	struct timespec Timeout;

	Timeout.tv_sec = usec / 1000000;
	Timeout.tv_nsec = (usec % 1000000) * 1000;

	sigwrap_InitTimeout(&To, &Timeout);

	while (1)
	{
		if (usleep(usec) == 0)
			return 0;

		if (errno != EINTR)
			return -1;

		if (sigwrap_CheckTimeout(&To))
			return 0;

		usec = To.Timeout.tv_sec * 1000000 + To.Timeout.tv_nsec / 1000;
	}
}


int sigwrap_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	SIGWRAP_TIMEOUT To;

	if (timeout > 0)
	{
		struct timespec Timeout;

		Timeout.tv_sec = timeout / 1000;
		Timeout.tv_nsec = (timeout % 1000) * 1000000;

		sigwrap_InitTimeout(&To, &Timeout);
	}

	while (1)
	{
		int Result = poll(fds, nfds, timeout);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;

		if (timeout < 0) // Specifying a negative value in timeout means an infinite/no timeout. 
			continue;
		else if (timeout == 0)
			continue; // We want to make sure that at least one check was not aborted with EINTR

		if (sigwrap_CheckTimeout(&To))
			return 0;

		timeout = To.Timeout.tv_sec * 1000 + To.Timeout.tv_nsec / 1000000;
	}
}

int sigwrap_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	while (1)
	{
		int Result = select(nfds, readfds, writefds, exceptfds, timeout);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


int sigwrap_pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout,
		const sigset_t *sigmask)
{
	SIGWRAP_TIMEOUT To;

	if (timeout != NULL)
	{
		sigwrap_InitTimeout(&To, timeout);
		timeout = &To.Timeout;
	}

	while (1)
	{
		int Result = pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;

		if (timeout == NULL)
			continue;

		if (sigwrap_CheckTimeout(&To))
			return 0;
	}
}


int sigwrap_msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg)
{
	while (1)
	{
		int Result = msgsnd(msqid, msgp, msgsz, msgflg);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


ssize_t sigwrap_msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
	while (1)
	{
		ssize_t Result = msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


int sigwrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	while (1)
	{
		int Result = connect(sockfd, addr, addrlen);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


ssize_t sigwrap_send(int sockfd, const void *buf, size_t len, int flags)
{
	while (1)
	{
		ssize_t Result = send(sockfd, buf, len, flags);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


ssize_t sigwrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr,
		socklen_t addrlen)
{
	while (1)
	{
		ssize_t Result = sendto(sockfd, buf, len, flags, dest_addr, addrlen);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


ssize_t sigwrap_sendsendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	while (1)
	{
		ssize_t Result = sendmsg(sockfd, msg, flags);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}


int sigwrap_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	while (1)
	{
		int Result = accept(sockfd, addr, addrlen);

		if (Result != -1)
			return Result;

		if (errno != EINTR)
			return Result;
	}
}

// EINTR wrapper for the standard read() function. Can be used for sockets that are the to non-blocking mode.
// The length of the returned data can be shorter than the requested one!

ssize_t sigwrap_read(int fd, void *buf, size_t count)
{
	while (1)
	{
		ssize_t Result = read(fd, buf, count);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


// EINTR wrapper for the standard read() function. Waits until ALL requested data is available. Use the non-blocking version (sigwrap_read)
// for sockets that are set to non-blocking mode or when partial data is okay
// Although the description for the read() function describes it differently, it seems possible that the original function may already return
// even though partial data has already been read. This implementation makes sure that all requested data have been read.
// See the comment in the signal description https://linux.die.net/man/7/signal
//* read(2), readv(2), write(2), writev(2), and ioctl(2) calls on "slow" devices. 
//* A "slow" device is one where the I/O call may block for an indefinite time, for example, a terminal, pipe, or socket. 
//* (A disk is not a slow device according to this definition.) If an I/O call on a slow device has already transferred
//* some data by the time it is interrupted by a signal handler, then the call will return a success status (normally, the number of bytes transferred). 

ssize_t sigwrap_blocking_read(int hFile, void *pData, size_t RdLen)
{
	ssize_t Transfered;
	ssize_t Len = RdLen;

	while ((Transfered = read(hFile, pData, Len)) != Len)
	{
		if (Transfered == 0) // EOF reached?
			return 0;

		if (Transfered != -1)
		{
			pData += Transfered;
			Len -= Transfered;
			continue;
		}

		if (errno != EINTR)
			return -1;
	}

	return RdLen;
}


ssize_t sigwrap_readv(int fd, const struct iovec *iov, int iovcnt)
{
	while (1)
	{
		ssize_t Result = readv(fd, iov, iovcnt);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


ssize_t sigwrap_recv(int sockfd, void *buf, size_t len, int flags)
{
	while (1)
	{
		ssize_t Result = recv(sockfd, buf, len, flags);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


ssize_t sigwrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	while (1)
	{
		ssize_t Result = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


ssize_t sigwrap_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	while (1)
	{
		ssize_t Result = recvmsg(sockfd, msg, flags);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


// EINTR wrapper for the standard write() function. Can be used for sockets that are the to non-blocking mode.
// The length of the effectively written data can be shorter than the length specified at the function call!

ssize_t sigwrap_write(int fd, const void *buf, size_t count)
{
	while (1)
	{
		ssize_t Result = write(fd, buf, count);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}

// EINTR wrapper for the standard write() function. Waits until ALL data is written! Use the non-blocking version (sigwrap_write)
// for sockets that are set to non-blocking mode, or when it is OK to write only partial data.
// Although the description for the write() function describes it differently, it seems possible that the original function may already return
// even though partial data has already been written. This implementation makes sure that all requested data have been written.
// See the comment in the signal description https://linux.die.net/man/7/signal
//* read(2), readv(2), write(2), writev(2), and ioctl(2) calls on "slow" devices. 
//* A "slow" device is one where the I/O call may block for an indefinite time, for example, a terminal, pipe, or socket. 
//* (A disk is not a slow device according to this definition.) If an I/O call on a slow device has already transferred
//* some data by the time it is interrupted by a signal handler, then the call will return a success status (normally, the number of bytes transferred). 

ssize_t sigwrap_blocking_write(int hFile, const void *pData, ssize_t WrtLen)
{
	ssize_t Written;
	ssize_t Len = WrtLen;

	while ((Written = write(hFile, pData, Len)) != Len)
	{
		if (Written != -1)
		{
			pData += Written;
			Len -= Written;
			continue;
		}

		if (errno != EINTR)
			return -1;
	}

	return WrtLen;
}


ssize_t sigwrap_writev(int fd, const struct iovec *iov, int iovcnt)
{
	while (1)
	{
		ssize_t Result = writev(fd, iov, iovcnt);

		if (Result != -1)
			return (Result);

		if (errno != EINTR)
			return (Result);
	}
}


int sigwrap_close(int hFile)
{
	while (close(hFile) == -1)
	{
		if (errno != EINTR)
			return -1;
	}

	return 0;
}


int sigwrap_open_mode(const char *pathname, int flags, mode_t mode)
{
	while (1)
	{
		int hFile = open(pathname, flags, mode);

		if(hFile != -1)
			return hFile;

		if (errno != EINTR)
			return hFile;
	}
}

int sigwrap_open(const char *pathname, int flags)
{
	while (1)
	{
		int hFile = open(pathname, flags);

		if(hFile != -1)
			return hFile;

		if (errno != EINTR)
			return hFile;
	}
}


pid_t sigwrap_wait(int *status)
{
	while(1)
	{
		pid_t Result = wait(status);

		if(Result != -1)
			return Result;

		if(errno != EINTR)
			return Result;
	}
}


pid_t sigwrap_waitpid(pid_t pid, int *status, int options)
{
	while(1)
	{
		pid_t Result = waitpid(pid, status, options);

		if(Result != -1)
			return Result;

		if(errno != EINTR)
			return Result;
	}
}


int sigwrap_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	while(1)
	{
		int Result = waitid(idtype, id, infop, options);

		if(Result != -1)
			return Result;

		if(errno != EINTR)
			return Result;
	}
}


int sigwrap_flock(int fd, int operation) 
{
	while(1)
	{
		int Result = flock(fd, operation);

		if(Result != -1)
			return Result;

		if(errno != EINTR)
			return Result;
	}
}


