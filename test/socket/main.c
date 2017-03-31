#include <stdio.h>
#include <errno.h>

#include <rtl_event.h>
#include <rtl_socket.h>

static void on_read(int fd, void *args)
{
	int n;
	char buf[12];

	printf("on_read\n");

	for (;;) {
		n = rtl_socket_recv(fd, buf, sizeof(buf));
		if (n < 0) {
			printf("received failed\n");
			break;
		} else if (n == 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("recv done\n");
				break;
			}
		} else {
			buf[n] = '\0';
			printf("received: %s\n", buf);
		}
	}
}

static void on_error(int fd, void *args)
{
	printf("on_error\n");
}

static void on_close(int fd, void *args)
{
	printf("on_close\n");
	rtl_socket_close(fd);
}

static void on_accept(int fd, void *args)
{
	int conn_fd;
	rtl_socket_addr_t addr;
	char ip[INET_ADDRSTRLEN];

	struct rtl_event_base *event_base = (struct rtl_event_base *)args;

	conn_fd = rtl_socket_accept(fd, &addr.ip, &addr.port);
	if (conn_fd < 0)
		return;
	rtl_socket_set_nonblock(conn_fd);

	rtl_socket_addr_ntop(ip, addr.ip);
    printf("new connection: ip = %s, port = %d\n", ip, addr.port);

	struct rtl_event *e = rtl_event_create(conn_fd, on_read, NULL, on_error, on_close, NULL);
	if (rtl_event_add(event_base, e) < 0) {
		rtl_socket_close(conn_fd);
		return;
	}
}

int main(int argc, char **argv)
{
	int listen_fd;

	listen_fd = rtl_socket_tcp_bind_listen(NULL, 1991);
	if (listen_fd < 0)
		return 1;

    struct rtl_event_base *event_base = rtl_event_base_create();
    if (!event_base) {
        fprintf(stderr, "rtl_event_base_create failed!\n");
        return -1;
    }

    struct rtl_event *event;
	event = rtl_event_create(listen_fd, on_accept, NULL, on_error, NULL, event_base);
    if (!event) {
        fprintf(stderr, "rtl_event_create failed!\n");
        return -1;
    }
    if (rtl_event_add(event_base, event) < 0) {
        fprintf(stderr, "rtl_event_add failed!\n");
        return -1;
    }
    rtl_event_base_loop(event_base);

    return 0;
}
