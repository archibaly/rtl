#ifndef _RTL_HTTP_REQ_H_
#define _RTL_HTTP_REQ_H_

#include "rtl_http_hdr.h"
#include "rtl_socket.h"

typedef enum {
	RTL_HTTP_REQ_TYPE_GET = 0,
	RTL_HTTP_REQ_TYPE_OPTIONS,
	RTL_HTTP_REQ_TYPE_HEAD,
	RTL_HTTP_REQ_TYPE_POST,
	RTL_HTTP_REQ_TYPE_PUT,
	RTL_HTTP_REQ_TYPE_DELETE,
	RTL_HTTP_REQ_TYPE_TRACE,
	RTL_HTTP_REQ_TYPE_CONNECT,
	RTL_HTTP_REQ_TYPE_PROPFIND,
	RTL_HTTP_REQ_TYPE_PROPPATCH,
	RTL_HTTP_REQ_TYPE_MKCOL,
	RTL_HTTP_REQ_TYPE_COPY,
	RTL_HTTP_REQ_TYPE_MOVE,
	RTL_HTTP_REQ_TYPE_LOCK,
	RTL_HTTP_REQ_TYPE_UNLOCK
} rtl_http_req_type_t;

extern const char *http_req_type_char[];

typedef struct {
	rtl_http_req_type_t type;
	char *host;
	int port;
	char *path;
	rtl_http_hdr_list_t *headers;
} rtl_http_req_t;

rtl_http_req_t *rtl_http_req_new(rtl_http_req_type_t type, const char *host, int port,
								 const char *path);
void rtl_http_req_destroy(rtl_http_req_t *req);
struct rtl_socket_connection *rtl_http_req_conn(rtl_http_req_t *req);
void rtl_http_req_discon(struct rtl_socket_connection *conn);
int rtl_http_req_send_hdr(const rtl_http_req_t *req,
						  struct rtl_socket_connection *conn,
						  int content_len);
int rtl_http_req_send_body(const rtl_http_req_t *req,
						   struct rtl_socket_connection *conn,
						   const unsigned char *body, int body_len);

#endif /* _RTL_HTTP_REQ_H_ */
