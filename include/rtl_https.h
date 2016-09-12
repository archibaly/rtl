#ifndef _RTL_HTTPS_H_
#define _RTL_HTTPS_H_

#include <stdint.h>

#include "rtl_http.h"

int rtl_https_send_request(int type, const char *host, uint16_t port, const char *path, char *resp, int len);

#endif /* _RTL_HTTPS_H_ */
