#ifndef ACCVM_FUNC_TAGS_H
#define ACCVM_FUNC_TAGS_H

// if a outlaw tag is 0, everyone should update it.

// regular file read/write never blocks.

// 1 for pipe, 2 for socket
#define TAG_READ_PP -1
#define TAG_WRITE_PP 1
#define TAG_READV_PP -1
#define TAG_WRITEV_PP 1
#define TAG_PREAD_PP -1
#define TAG_PREAD64_PP -1
#define TAG_PWRITE_PP 1
#define TAG_PWRITE64_PP 1

#define TAG_READ_SK -2
#define TAG_WRITE_SK 2
#define TAG_READV_SK -2
#define TAG_WRITEV_SK 2
#define TAG_PREAD_SK -2
#define TAG_PREAD64_SK -2
#define TAG_PWRITE_SK 2
#define TAG_PWRITE64_SK 2

#define TAG_RECV -2
#define TAG_RECVFROM -2
#define TAG_SEND 2
#define TAG_SENDTO 2

// both recv or send should update vts of those who are waiting
// the checking should be for y, x/100 > 0 && x%100 == 0 && (y == x/100 or -y == x/100)
#define TAG_EPOLL_WAIT_SK 200
#define TAG_SELECT_SK 200
#define TAG_POLL_SK 200

#define TAG_EPOLL_WAIT_PP 100
#define TAG_SELECT_PP 100
#define TAG_POLL_PP 100


#define TAG_SEM_WAIT -3
#define TAG_SEM_TIMEDWAIT -3
#define TAG_SEM_POST 3

#define TAG_PTHREAD_MUTEX_LOCK -4
#define TAG_PTHREAD_MUTEX_UNLOCK 4

#define TAG_PTHREAD_COND_WAIT -5
#define TAG_PTHREAD_COND_TIMEDWAIT -5
#define TAG_PTHREAD_COND_SIGNAL 5
#define TAG_PTHREAD_COND_BROADCAST 5

#define TAG_PTHREAD_RWLOCK_RDLOCK -6
#define TAG_PTHREAD_RWLOCK_WRLOCK -6
#define TAG_PTHREAD_RWLOCK_UNLOCK 6

#define TAG_CONNECT -10
#define TAG_ACCEPT 10
#define TAG_ACCEPT4 10

#endif