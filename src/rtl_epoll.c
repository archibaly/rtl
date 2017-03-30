#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>

#include "rtl_event.h"

#define EPOLL_MAX_NEVENT			512
#define MAX_SECONDS_IN_MSEC_LONG	(((LONG_MAX) - 999) / 1000)

struct epoll_ctx {
	int epfd;
	int nevents;
	struct epoll_event *events;
};

static void *epoll_init(void)
{
	int fd;
	struct epoll_ctx *ec;

	fd = epoll_create(1);
	if (fd < 0) {
		perror("epoll_create");
		return NULL;
	}

	ec = calloc(1, sizeof(struct epoll_ctx));
	if (!ec) {
		perror("calloc");
		goto err;
	}
	ec->epfd = fd;
	ec->nevents = EPOLL_MAX_NEVENT;
	ec->events = calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
	if (!ec->events) {
		perror("calloc");
		free(ec);
		goto err;
	}
	return ec;

err:
	close(fd);
	return NULL;
}

static void epoll_deinit(void *ctx)
{
	struct epoll_ctx *ec = (struct epoll_ctx *)ctx;
	if (!ec)
		return;

	free(ec->events);
	free(ec);
}

static int epoll_add(struct rtl_event_base *eb, struct rtl_event *e)
{
	struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;
	struct epoll_event epev;

	memset(&epev, 0, sizeof(epev));
	if (e->flags & EVENT_READ)
		epev.events |= EPOLLIN;
	if (e->flags & EVENT_WRITE)
		epev.events |= EPOLLOUT;
	if (e->flags & EVENT_ERROR)
		epev.events |= EPOLLERR;
	if (e->flags & EVENT_CLOSED)
		epev.events |= EPOLLRDHUP;
	epev.events |= EPOLLET;
	epev.data.ptr = (void *)e;

	if (epoll_ctl(ec->epfd, EPOLL_CTL_ADD, e->evfd, &epev) < 0) {
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}

static int epoll_del(struct rtl_event_base *eb, struct rtl_event *e)
{
	struct epoll_ctx *ec = (struct epoll_ctx *)eb->ctx;
	if (epoll_ctl(ec->epfd, EPOLL_CTL_DEL, e->evfd, NULL) < 0) {
		perror("epoll_ctl");
		return -1;
	}
	return 0;
}

static int epoll_dispatch(struct rtl_event_base *eb, struct timeval *tv)
{
	struct epoll_ctx *epop = (struct epoll_ctx *)eb->ctx;
	struct epoll_event *events = epop->events;
	int i, n;
	int timeout = -1;

	if (tv != NULL) {
		if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
			timeout = -1;
		else
			timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
	} else {
		timeout = -1;
	}
	n = epoll_wait(epop->epfd, events, epop->nevents, timeout);
	if (n == -1) {
		if (errno != EINTR) {
			perror("epoll_wait");
			return -1;
		}
		return 0;
	}
	if (n == 0) {
		printf("epoll_wait timeout\n");
		return 0;
	}
	for (i = 0; i < n; i++) {
		int what = events[i].events;
		struct rtl_event *e = (struct rtl_event *)events[i].data.ptr;

		if (what & EPOLLIN)
			e->evcb->ev_in(e, (void *)e->evcb->args);
		if (what & EPOLLOUT)
			e->evcb->ev_out(e, (void *)e->evcb->args);
		if (what & EPOLLERR)
			e->evcb->ev_err(e, (void *)e->evcb->args);
	}
	return 0;
}

struct rtl_event_ops rtl_epoll_ops = {
	.init     = epoll_init,
	.deinit   = epoll_deinit,
	.add      = epoll_add,
	.del      = epoll_del,
	.dispatch = epoll_dispatch,
};
