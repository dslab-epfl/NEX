#ifndef ACCVM_NETWORK_H
#define ACCVM_NETWORK_H
#include <accvm/accvm.h>
#include <accvm/context.h>
#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <poll.h>

// read/write
extern ssize_t (*orig_read)(int fd, void *buf, size_t count);
extern ssize_t (*orig_readv)(int fd, const struct iovec *iov, int iovcnt);
extern ssize_t (*orig_pread)(int fd, void *buf, size_t count, off_t offset);
extern ssize_t (*orig_pread64)(int fd, void *buf, size_t count, off_t offset);
// ssize_t (*orig_preadv)(int fd, const struct iovec *iov, int iovcnt, off_t offset);
// ssize_t (*orig_preadv2)(int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags);
extern ssize_t (*orig_pwrite)(int fd, const void *buf, size_t count, off_t offset);
extern ssize_t (*orig_pwrite64)(int fd, const void *buf, size_t count, off_t offset);
extern ssize_t (*orig_write)(int fd, const void *buf, size_t count);
extern ssize_t (*orig_writev)(int fd, const struct iovec *iov, int iovcnt);

extern int (*orig_accept4) (int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
extern int (*orig_epoll_wait)(int epfd, struct epoll_event *events, int maxevents, int timeout);
extern int (*orig_select)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
extern int (*orig_poll)(struct pollfd *fds, nfds_t nfds, int timeout);

extern ssize_t (*orig_recvfrom)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern ssize_t (*orig_sendto)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern ssize_t (*orig_send)(int sockfd, const void *buf, size_t len, int flags);
extern ssize_t (*orig_recv)(int sockfd, void *buf, size_t len, int flags);
extern int (*orig_listen)(int sockfd, int backlog);
extern int (*orig_connect)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int (*orig_accept)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int host_id;

#endif