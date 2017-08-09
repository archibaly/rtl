#ifndef _RTL_HTTPS_RESP_H_
#define _RTL_HTTPS_RESP_H_

#include "rtl_http_resp.h"
#include "rtl_https_req.h"

int rtl_https_resp_read_hdrs(rtl_http_resp_t *resp,
							 struct rtl_https_connection *conn);
int rtl_https_resp_read_body(rtl_http_resp_t *resp,
							 struct rtl_https_connection *conn,
							 unsigned char *buf, size_t size);
int rtl_https_resp_save_body_to_file(rtl_http_resp_t *resp,
									 struct rtl_https_connection *conn,
									 const char *filename);

#endif /* _RTL_HTTPS_RESP_H_ */
