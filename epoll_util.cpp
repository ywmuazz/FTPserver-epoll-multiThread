#include "epoll_util.h"


int setnonblocking(int fd) { /* 将文件描述符设置为非阻塞 */
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; /* ET触发 */
    if (one_shot) {
        event.events |= EPOLLONESHOT; /* 这里特别要注意一下 */
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void addFdData(int epollfd, int fd, void* p,bool one_shot) {
    epoll_event event;
    event.data.ptr = p;
    event.events = EPOLLIN | EPOLLET |EPOLLHUP; /* ET触发 */
    if (one_shot) {
        event.events |= EPOLLONESHOT; /* 这里特别要注意一下 */
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}


void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET; /* ET触发 */
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int Epoll_create(int size) {
    int rc;
    if ((rc = epoll_create(size)) < 0) {
        unix_error("epoll_crate failed");
    }
    return rc; /* 否则的话,返回文件描述符 */
}

int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout) {
    int rc;
    if ((rc = epoll_wait(epfd, events, maxevents, timeout)) == -1) {
        unix_error("epoll failed");
    }
    return rc;
}
