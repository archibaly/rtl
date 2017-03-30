#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "rtl_event.h"

extern const struct rtl_event_ops rtl_epoll_ops;

static const struct rtl_event_ops *event_ops[] = {
	&rtl_epoll_ops,
	NULL
};

static void event_in(struct rtl_event *event, void *args)
{
}

struct rtl_event_base *rtl_event_base_create(void)
{
	int i;
	int fds[2];
	struct rtl_event_base *eb = NULL;

	if (pipe(fds)) {
		perror("pipe failed");
		return NULL;
	}
	eb = calloc(1, sizeof(struct rtl_event_base));
	if (!eb) {
		fprintf(stderr, "malloc rtl_event_base failed!\n");
		close(fds[0]);
		close(fds[1]);
		return NULL;
	}

	for (i = 0; event_ops[i]; i++) {
		eb->ctx = event_ops[i]->init();
		eb->evop = event_ops[i];
	}
	eb->loop = 1;
	eb->rfd = fds[0];
	eb->wfd = fds[1];
	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK);

	struct rtl_event *e = rtl_event_create(eb->rfd, event_in, NULL, NULL, NULL);
	rtl_event_add(eb, e);

	return eb;
}

void rtl_event_base_destroy(struct rtl_event_base *eb)
{
	if (!eb)
		return;
	rtl_event_base_loop_break(eb);
	close(eb->rfd);
	close(eb->wfd);
	eb->evop->deinit(eb->ctx);
	free(eb);
}

int rtl_event_base_loop(struct rtl_event_base *eb)
{
	const struct rtl_event_ops *evop = eb->evop;
	int ret;
	while (eb->loop) {
		ret = evop->dispatch(eb, NULL);
		if (ret < 0) {
			fprintf(stderr, "dispatch failed\n");
		}
	}

	return 0;
}

int rtl_event_base_wait(struct rtl_event_base *eb)
{
	const struct rtl_event_ops *evop = eb->evop;
	return evop->dispatch(eb, NULL);
}

void rtl_event_base_loop_break(struct rtl_event_base *eb)
{
	char buf[1];
	buf[0] = 0;
	eb->loop = 0;
	if (1 != write(eb->wfd, buf, 1))
		perror("write error");
}

void rtl_event_base_signal(struct rtl_event_base *eb)
{
	char buf[1];
	buf[0] = 0;
	if (1 != write(eb->wfd, buf, 1))
		perror("write error");
}

struct rtl_event *rtl_event_create(int fd,
		void (*ev_in)(struct rtl_event *, void *),
		void (*ev_out)(struct rtl_event *, void *),
		void (*ev_err)(struct rtl_event *, void *),
		void *args)
{
	int flags = 0;
	struct rtl_event *e = calloc(1, sizeof(struct rtl_event));
	if (!e) {
		fprintf(stderr, "calloc rtl_event failed!\n");
		return NULL;
	}
	struct rtl_event_cbs *evcb = calloc(1, sizeof(struct rtl_event_cbs));
	if (!evcb) {
		fprintf(stderr, "calloc rtl_event failed!\n");
		free(e);
		return NULL;
	}
	evcb->ev_in = ev_in;
	evcb->ev_out = ev_out;
	evcb->ev_err = ev_err;
	evcb->args = args;
	if (ev_in)
		flags |= EVENT_READ;
	if (ev_out)
		flags |= EVENT_WRITE;
	if (ev_err)
		flags |= EVENT_ERROR;

	e->evfd = fd;
	e->flags = flags;
	e->evcb = evcb;

	return e;
}

void rtl_event_destroy(struct rtl_event *e)
{
	if (!e)
		return;
	free(e->evcb);
	close(e->evfd);
	free(e);
}

int rtl_event_add(struct rtl_event_base *eb, struct rtl_event *e)
{
	if (!e || !eb) {
#ifdef DEBUG
		fprintf(stderr, "%s:%d paraments is NULL\n", __func__, __LINE__);
#endif
		return -1;
	}
	return eb->evop->add(eb, e);
}

int rtl_event_del(struct rtl_event_base *eb, struct rtl_event *e)
{
	if (!e || !eb) {
#ifdef DEBUG
		fprintf(stderr, "%s:%d paraments is NULL\n", __func__, __LINE__);
#endif
		return -1;
	}
	return eb->evop->del(eb, e);
}
