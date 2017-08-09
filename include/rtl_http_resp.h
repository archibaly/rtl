#ifndef _RTL_HTTP_RESP_H_
#define _RTL_HTTP_RESP_H_

#include "rtl_http_hdr.h"
#include "rtl_socket.h"

#define RTL_HTTP_RESP_INFORMATIONAL(x) (x >=100 && < 200)
#define RTL_HTTP_RESP_SUCCESS(x) (x >= 200 && x < 300)
#define RTL_HTTP_RESP_REDIR(x) (x >= 300 && x < 400)
#define RTL_HTTP_RESP_CLIENT_ERR(x) (x >= 400 && x < 500)

#define RTL_HTTP_RESP_BUF_SIZE	4096

typedef struct {
	int status_code;
	char *reason_phrase;
	rtl_http_hdr_list_t *headers;
	unsigned char buf[RTL_HTTP_RESP_BUF_SIZE];
	unsigned char *buf_ptr;
	int buf_remain;
} rtl_http_resp_t;

rtl_http_resp_t *rtl_http_resp_new(void);
void rtl_http_resp_destroy(rtl_http_resp_t *resp);

/* Must call this before any function below */
int rtl_http_resp_read_hdrs(rtl_http_resp_t *resp,
							struct rtl_socket_connection *conn);

int rtl_http_resp_get_status_code(rtl_http_resp_t *resp);
char *rtl_http_resp_get_reason_phrase(rtl_http_resp_t *resp);
rtl_http_hdr_list_t *rtl_http_resp_get_hdrs(rtl_http_resp_t *resp);

/* We may call this function servel times to read all response data */
int rtl_http_resp_read_body(rtl_http_resp_t *resp,
							struct rtl_socket_connection *conn,
							unsigned char *buf, size_t size);

int rtl_http_resp_save_body_to_file(rtl_http_resp_t *resp,
									struct rtl_socket_connection *conn,
									const char *filename);

#endif /* _RTL_HTTP_RESP_H_ */
