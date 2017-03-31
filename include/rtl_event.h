#ifndef _RTL_EVENT_H_
#define _RTL_EVENT_H_

#include <sys/time.h>

enum rtl_event_flags {
    EVENT_TIMEOUT  = 1<<0,
    EVENT_READ     = 1<<1,
    EVENT_WRITE    = 1<<2,
    EVENT_SIGNAL   = 1<<3,
    EVENT_PERSIST  = 1<<4,
    EVENT_ET       = 1<<5,
    EVENT_FINALIZE = 1<<6,
    EVENT_CLOSED   = 1<<7,
    EVENT_ERROR    = 1<<8,
    EVENT_EXCEPT   = 1<<9,
};

struct rtl_event_cbs {
    void (*ev_in)(int fd, void *args);
    void (*ev_out)(int fd, void *args);
    void (*ev_err)(int fd, void *args);
    void (*ev_close)(int fd, void *args);
    void *args;
};

struct rtl_event {
    int evfd;
    int flags;
    struct rtl_event_cbs *evcb;
};

struct rtl_event_base;
struct rtl_event_ops {
    void *(*init)(void);
    void (*deinit)(void *ctx);
    int (*add)(struct rtl_event_base *eb, struct rtl_event *e);
    int (*del)(struct rtl_event_base *eb, struct rtl_event *e);
    int (*dispatch)(struct rtl_event_base *eb, struct timeval *tv);
};

struct rtl_event_base {
    /* pointer to backend-specific data */
    void *ctx;
    int loop;
    int rfd;
    int wfd;
    const struct rtl_event_ops *evop;
};

struct rtl_event_base *rtl_event_base_create(void);
void rtl_event_base_destroy(struct rtl_event_base *);
int rtl_event_base_loop(struct rtl_event_base *);
void rtl_event_base_loop_break(struct rtl_event_base *);
int rtl_event_base_wait(struct rtl_event_base *eb);
void rtl_event_base_signal(struct rtl_event_base *eb);

struct rtl_event *rtl_event_create(int fd,
		void (ev_in)(int, void *),
		void (ev_out)(int, void *),
		void (ev_err)(int, void *),
		void (ev_close)(int, void *),
		void *args);

void rtl_event_destroy(struct rtl_event *e);
int rtl_event_add(struct rtl_event_base *eb, struct rtl_event *e);
int rtl_event_del(struct rtl_event_base *eb, struct rtl_event *e);

#endif /* _RTL_EVENT_H_ */
