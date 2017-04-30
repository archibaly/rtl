#ifndef _RTL_FCGI_H_
#define _RTL_FCGI_H_

#include <stdint.h>

#include "rtl_hash.h"

#define RTL_FCGI_VERSION_1	1
#define RTL_FCGI_MAX_LENGTH	0xffff

typedef enum rtl_fcgi_role {
	RTL_FCGI_RESPONDER	= 1,
	RTL_FCGI_AUTHORIZER	= 2,
	RTL_FCGI_FILTER		= 3
} rtl_fcgi_role_t;

typedef enum rtl_fcgi_request_type {
	RTL_FCGI_BEGIN_REQUEST		=  1, /* [in]                              */
	RTL_FCGI_ABORT_REQUEST		=  2, /* [in]  (not supported)             */
	RTL_FCGI_END_REQUEST		=  3, /* [out]                             */
	RTL_FCGI_PARAMS				=  4, /* [in]  environment variables       */
	RTL_FCGI_STDIN				=  5, /* [in]  post data                   */
	RTL_FCGI_STDOUT				=  6, /* [out] response                    */
	RTL_FCGI_STDERR				=  7, /* [out] errors                      */
	RTL_FCGI_DATA				=  8, /* [in]  filter data (not supported) */
	RTL_FCGI_GET_VALUES			=  9, /* [in]                              */
	RTL_FCGI_GET_VALUES_RESULT	= 10  /* [out]                             */
} rtl_fcgi_request_type_t;

typedef enum rtl_fcgi_protocol_status {
	RTL_FCGI_REQUEST_COMPLETE	= 0,
	RTL_FCGI_CANT_MPX_CONN		= 1,
	RTL_FCGI_OVERLOADED			= 2,
	RTL_FCGI_UNKNOWN_ROLE		= 3
} rtl_fcgi_protocol_status_t;

struct rtl_fcgi_param {
	unsigned char key_len;
	unsigned char value_len;
	char *key;
	char *value;
};

typedef struct rtl_fcgi {
	int listen_sock;
	int conn_sock;
	int id;
	int keep;

	struct rtl_hash_table *env;

	int in_len;
	unsigned char *in_buf;
} rtl_fcgi_t;

rtl_fcgi_t *rtl_fcgi_init(const char *path, uint16_t port);
int rtl_fcgi_accept(rtl_fcgi_t *rtl_fcgi);
void rtl_fcgi_finish(rtl_fcgi_t *rtl_fcgi);
int rtl_fcgi_printf(rtl_fcgi_t *rtl_fcgi, const char *fmt, ...);
int rtl_fcgi_write(rtl_fcgi_t *rtl_fcgi, const void *buf, size_t count);
unsigned char *rtl_fcgi_get_stdin(rtl_fcgi_t *rtl_fcgi, int *len);
char *rtl_fcgi_getenv(const rtl_fcgi_t *rtl_fcgi, const char *name);
int rtl_fcgi_putenv(rtl_fcgi_t *rtl_fcgi, const char *key, const char *value);
int rtl_fcgi_read_request(rtl_fcgi_t *rtl_fcgi);

#endif /* _RTL_FCGI_H_ */
