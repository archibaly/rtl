#ifndef _RTL_HTTP_HDR_H_
#define _RTL_HTTP_HDR_H_

#define RTL_HTTP_HDRS_MAX	128

#include <stddef.h>

extern const char RTL_HTTP_HDR_Allow[];
extern const char RTL_HTTP_HDR_Content_Encoding[];
extern const char RTL_HTTP_HDR_Content_Language[];
extern const char RTL_HTTP_HDR_Content_Length[];
extern const char RTL_HTTP_HDR_Content_Location[];
extern const char RTL_HTTP_HDR_Content_MD5[];
extern const char RTL_HTTP_HDR_Content_Range[];
extern const char RTL_HTTP_HDR_Content_Type[];
extern const char RTL_HTTP_HDR_Expires[];
extern const char RTL_HTTP_HDR_Last_Modified[];

extern const char RTL_HTTP_HDR_Cache_Control[];
extern const char RTL_HTTP_HDR_Connection[];
extern const char RTL_HTTP_HDR_Date[];
extern const char RTL_HTTP_HDR_Pragma[];
extern const char RTL_HTTP_HDR_Transfer_Encoding[];
extern const char RTL_HTTP_HDR_Update[];
extern const char RTL_HTTP_HDR_Trailer[];
extern const char RTL_HTTP_HDR_Via[];

extern const char RTL_HTTP_HDR_Accept[];
extern const char RTL_HTTP_HDR_Accept_Charset[];
extern const char RTL_HTTP_HDR_Accept_Encoding[];
extern const char RTL_HTTP_HDR_Accept_Language[];
extern const char RTL_HTTP_HDR_Authorization[];
extern const char RTL_HTTP_HDR_Expect[];
extern const char RTL_HTTP_HDR_From[];
extern const char RTL_HTTP_HDR_Host[];
extern const char RTL_HTTP_HDR_If_Modified_Since[];
extern const char RTL_HTTP_HDR_If_Match[];
extern const char RTL_HTTP_HDR_If_None_Match[];
extern const char RTL_HTTP_HDR_If_Range[];
extern const char RTL_HTTP_HDR_If_Unmodified_Since[];
extern const char RTL_HTTP_HDR_Max_Forwards[];
extern const char RTL_HTTP_HDR_Proxy_Authorization[];
extern const char RTL_HTTP_HDR_Range[];
extern const char RTL_HTTP_HDR_Referrer[];
extern const char RTL_HTTP_HDR_TE[];
extern const char RTL_HTTP_HDR_User_Agent[];

extern const char RTL_HTTP_HDR_Accept_Ranges[];
extern const char RTL_HTTP_HDR_Age[];
extern const char RTL_HTTP_HDR_ETag[];
extern const char RTL_HTTP_HDR_Location[];
extern const char RTL_HTTP_HDR_Retry_After[];
extern const char RTL_HTTP_HDR_Server[];
extern const char RTL_HTTP_HDR_Vary[];
extern const char RTL_HTTP_HDR_Warning[];
extern const char RTL_HTTP_HDR_WWW_Authenticate[];

extern const char RTL_HTTP_HDR_Set_Cookie[];

extern const char RTL_HTTP_HDR_DAV[];
extern const char RTL_HTTP_HDR_Depth[];
extern const char RTL_HTTP_HDR_Destination[];
extern const char RTL_HTTP_HDR_If[];
extern const char RTL_HTTP_HDR_Lock_Token[];
extern const char RTL_HTTP_HDR_Overwrite[];
extern const char RTL_HTTP_HDR_Status_URI[];
extern const char RTL_HTTP_HDR_Timeout[];

/* a header list */
typedef struct {
	char *header[RTL_HTTP_HDRS_MAX];
	char *value[RTL_HTTP_HDRS_MAX];
} rtl_http_hdr_list_t;

/* check to see if the library knows about the header */
const char *rtl_http_hdr_is_known(const char *hdr);

/* create a new list */
rtl_http_hdr_list_t *rtl_http_hdr_list_new(void);

/* destroy a list */
void rtl_http_hdr_list_destroy(rtl_http_hdr_list_t *list);

/* set a value in a list */
int rtl_http_hdr_set_value(rtl_http_hdr_list_t *list,
						   const char *name, const char *val);

/* set the value in a list from a range, not a NTS */
int rtl_http_hdr_set_value_no_nts(rtl_http_hdr_list_t *list,
								  const char *name_start,
								  int name_len,
								  const char *val_start, int val_len);

/* get a copy of a value in a list */
char *rtl_http_hdr_get_value(const rtl_http_hdr_list_t *list, const char *name);

/* get a copy of the headers in a list */
int rtl_http_hdr_get_headers(const rtl_http_hdr_list_t *list, char ***a_names, int *a_num_names);

int rtl_http_hdr_to_string(char *str, size_t size, const rtl_http_hdr_list_t *list);

/* clear a header in a list */
int rtl_http_hdr_clear_value(rtl_http_hdr_list_t *list, const char *name);

#endif /* _RTL_HTTP_HDR_H_ */
