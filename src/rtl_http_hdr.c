#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtl_http_hdr.h"

/* entity headers */
const char RTL_HTTP_HDR_Allow[] = "Allow";
const char RTL_HTTP_HDR_Content_Encoding[] = "Content-Encoding";
const char RTL_HTTP_HDR_Content_Language[] = "Content-Language";
const char RTL_HTTP_HDR_Content_Length[] = "Content-Length";
const char RTL_HTTP_HDR_Content_Location[] = "Content-Location";
const char RTL_HTTP_HDR_Content_MD5[] = "Content-MD5";
const char RTL_HTTP_HDR_Content_Range[] = "Content-Range";
const char RTL_HTTP_HDR_Content_Type[] = "Content-Type";
const char RTL_HTTP_HDR_Expires[] = "Expires";
const char RTL_HTTP_HDR_Last_Modified[] = "Last-Modified";

/* general headers */
const char RTL_HTTP_HDR_Cache_Control[] = "Cache-Control";
const char RTL_HTTP_HDR_Connection[] = "Connection";
const char RTL_HTTP_HDR_Date[] = "Date";
const char RTL_HTTP_HDR_Pragma[] = "Pragma";
const char RTL_HTTP_HDR_Transfer_Encoding[] = "Transfer-Encoding";
const char RTL_HTTP_HDR_Update[] = "Update";
const char RTL_HTTP_HDR_Trailer[] = "Trailer";
const char RTL_HTTP_HDR_Via[] = "Via";

/* request headers */
const char RTL_HTTP_HDR_Accept[] = "Accept";
const char RTL_HTTP_HDR_Accept_Charset[] = "Accept-Charset";
const char RTL_HTTP_HDR_Accept_Encoding[] = "Accept-Encoding";
const char RTL_HTTP_HDR_Accept_Language[] = "Accept-Language";
const char RTL_HTTP_HDR_Authorization[] = "Authorization";
const char RTL_HTTP_HDR_Expect[] = "Expect";
const char RTL_HTTP_HDR_From[] = "From";
const char RTL_HTTP_HDR_Host[] = "Host";
const char RTL_HTTP_HDR_If_Modified_Since[] = "If-Modified-Since";
const char RTL_HTTP_HDR_If_Match[] = "If-Match";
const char RTL_HTTP_HDR_If_None_Match[] = "If-None-Match";
const char RTL_HTTP_HDR_If_Range[] = "If-Range";
const char RTL_HTTP_HDR_If_Unmodified_Since[] = "If-Unmodified-Since";
const char RTL_HTTP_HDR_Max_Forwards[] = "Max-Forwards";
const char RTL_HTTP_HDR_Proxy_Authorization[] = "Proxy-Authorization";
const char RTL_HTTP_HDR_Range[] = "Range";
const char RTL_HTTP_HDR_Referrer[] = "Referrer";
const char RTL_HTTP_HDR_TE[] = "TE";
const char RTL_HTTP_HDR_User_Agent[] = "User-Agent";

/* response headers */
const char RTL_HTTP_HDR_Accept_Ranges[] = "Accept-Ranges";
const char RTL_HTTP_HDR_Age[] = "Age";
const char RTL_HTTP_HDR_ETag[] = "ETag";
const char RTL_HTTP_HDR_Location[] = "Location";
const char RTL_HTTP_HDR_Retry_After[] = "Retry-After";
const char RTL_HTTP_HDR_Server[] = "Server";
const char RTL_HTTP_HDR_Vary[] = "Vary";
const char RTL_HTTP_HDR_Warning[] = "Warning";
const char RTL_HTTP_HDR_WWW_Authenticate[] = "WWW-Authenticate";

/* Other headers */
const char RTL_HTTP_HDR_Set_Cookie[] = "Set-Cookie";

/* WebDAV headers */
const char RTL_HTTP_HDR_DAV[] = "DAV";
const char RTL_HTTP_HDR_Depth[] = "Depth";
const char RTL_HTTP_HDR_Destination[] = "Destination";
const char RTL_HTTP_HDR_If[] = "If";
const char RTL_HTTP_HDR_Lock_Token[] = "Lock-Token";
const char RTL_HTTP_HDR_Overwrite[] = "Overwrite";
const char RTL_HTTP_HDR_Status_URI[] = "Status-URI";
const char RTL_HTTP_HDR_Timeout[] = "Timeout";

static const char *http_hdr_known_list[] = {
	/* entity headers */
	RTL_HTTP_HDR_Allow,
	RTL_HTTP_HDR_Content_Encoding,
	RTL_HTTP_HDR_Content_Language,
	RTL_HTTP_HDR_Content_Length,
	RTL_HTTP_HDR_Content_Location,
	RTL_HTTP_HDR_Content_MD5,
	RTL_HTTP_HDR_Content_Range,
	RTL_HTTP_HDR_Content_Type,
	RTL_HTTP_HDR_Expires,
	RTL_HTTP_HDR_Last_Modified,
	/* general headers */
	RTL_HTTP_HDR_Cache_Control,
	RTL_HTTP_HDR_Connection,
	RTL_HTTP_HDR_Date,
	RTL_HTTP_HDR_Pragma,
	RTL_HTTP_HDR_Transfer_Encoding,
	RTL_HTTP_HDR_Update,
	RTL_HTTP_HDR_Trailer,
	RTL_HTTP_HDR_Via,
	/* request headers */
	RTL_HTTP_HDR_Accept,
	RTL_HTTP_HDR_Accept_Charset,
	RTL_HTTP_HDR_Accept_Encoding,
	RTL_HTTP_HDR_Accept_Language,
	RTL_HTTP_HDR_Authorization,
	RTL_HTTP_HDR_Expect,
	RTL_HTTP_HDR_From,
	RTL_HTTP_HDR_Host,
	RTL_HTTP_HDR_If_Modified_Since,
	RTL_HTTP_HDR_If_Match,
	RTL_HTTP_HDR_If_None_Match,
	RTL_HTTP_HDR_If_Range,
	RTL_HTTP_HDR_If_Unmodified_Since,
	RTL_HTTP_HDR_Max_Forwards,
	RTL_HTTP_HDR_Proxy_Authorization,
	RTL_HTTP_HDR_Range,
	RTL_HTTP_HDR_Referrer,
	RTL_HTTP_HDR_TE,
	RTL_HTTP_HDR_User_Agent,
	/* response headers */
	RTL_HTTP_HDR_Accept_Ranges,
	RTL_HTTP_HDR_Age,
	RTL_HTTP_HDR_ETag,
	RTL_HTTP_HDR_Location,
	RTL_HTTP_HDR_Retry_After,
	RTL_HTTP_HDR_Server,
	RTL_HTTP_HDR_Vary,
	RTL_HTTP_HDR_Warning,
	RTL_HTTP_HDR_WWW_Authenticate,
	NULL
};

/* functions dealing with headers */
const char *rtl_http_hdr_is_known(const char *hdr)
{
	int pos = 0;
	const char *ret = NULL;

	if (!hdr)
		goto out;
	while (http_hdr_known_list[pos] != NULL) {
		if (strcasecmp(hdr, http_hdr_known_list[pos]) == 0) {
			ret = http_hdr_known_list[pos];
			break;
		}
		pos++;
	}
out:
	return ret;
}

rtl_http_hdr_list_t *rtl_http_hdr_list_new(void)
{
	return calloc(1, sizeof(rtl_http_hdr_list_t));
}

void rtl_http_hdr_list_destroy(rtl_http_hdr_list_t *list)
{
	int i = 0;

	if (list == NULL)
		return;
	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (list->header[i] && (rtl_http_hdr_is_known(list->header[i]) == NULL))
			free(list->header[i]);
		if (list->value[i])
			free(list->value[i]);
	}
	free(list);
}

int rtl_http_hdr_set_value_no_nts(rtl_http_hdr_list_t *list,
							  const char *name_start,
							  int name_len,
							  const char *val_start, int val_len)
{
	int ret = -1;
	char *tmp_name;
	char *tmp_val;

	/* note that a zero len value is valid... */
	if ((list == NULL) ||
		(name_start == NULL) || (val_start == NULL) || (name_len == 0))
		goto out;

	tmp_name = calloc(name_len + 1, sizeof(char));
	if (!tmp_name)
		goto out;
	memcpy(tmp_name, name_start, name_len);

	tmp_val = calloc(val_len + 1, sizeof(char));
	if (!tmp_val) {
		free(tmp_name);
		goto out;
	}
	memcpy(tmp_val, val_start, val_len);

	/* set the value */
	ret = rtl_http_hdr_set_value(list, tmp_name, tmp_val);
	free(tmp_name);
	free(tmp_val);
out:
	return ret;
}

int rtl_http_hdr_set_value(rtl_http_hdr_list_t *list,
					   const char *name, const char *val)
{
	int i = 0;
	char *tmp_value = NULL;
	int ret = -1;

	if ((list == NULL) || (name == NULL) || (val == NULL))
		goto out;
	tmp_value = rtl_http_hdr_get_value(list, name);
	if (tmp_value == NULL) {
		for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
			if (list->header[i] == NULL) {
				/* I promise not to mess with this value. */
				tmp_value = (char *)rtl_http_hdr_is_known(name);
				if (tmp_value) {
					list->header[i] = tmp_value;
					/* dont free this later... */
				} else {
					list->header[i] = strdup(name);
					if (list->header[i] == NULL)
						goto out;
				}
				list->value[i] = strdup(val);
				if (list->value[i])
					ret = 0;
				break;
			}
		}
	} else {
		for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
			if (list->value[i] == tmp_value) {
				free(list->value[i]);
				list->value[i] = strdup(val);
				if (list->value[i])
					ret = 0;
				break;
			}
		}
	}
out:
	return ret;
}

char *rtl_http_hdr_get_value(const rtl_http_hdr_list_t *list, const char *name)
{
	int i = 0;
	char *ret = NULL;

	if (name == NULL)
		goto out;
	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (list->header[i] && (strcasecmp(list->header[i], name) == 0)) {
			if (list->value[i] == NULL)
				goto out;
			ret = list->value[i];
			break;
		}
	}
out:
	return ret;
}

int rtl_http_hdr_get_headers(const rtl_http_hdr_list_t *list, char ***a_names, int *a_num_names)
{
	int i = 0;
	int num_names = 0;
	char **names;

	if (a_num_names == NULL)
		return -1;
	if (a_names == NULL)
		return -1;

	/* set our return values */
	*a_names = NULL;
	*a_num_names = 0;

	/* make a pass to find out how many headers we have. */
	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (list->header[i])
			num_names++;
	}

	/* return if there are no headers */
	if (num_names == 0)
		return 0;

	/* now that we know how many headers we have allocate the number of
	   slots in the return */
	names = calloc(num_names, sizeof(char *));
	if (names == NULL)
		return -1;

	/* copy the headers */
	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (list->header[i]) {
			names[i] = strdup(list->header[i]);
			if (names[i] == NULL)
				goto out;
		}
	}

	*a_names = names;
	*a_num_names = num_names;
	return 0;

	/* something bad happened.  Try to free up as much as possible. */
out:
	if (names) {
		for (i = 0; i < num_names; i++) {
			if (names[i]) {
				free(names[i]);
				names[i] = NULL;
			}
		}
		free(names);
		*a_names = 0;
	}
	*a_num_names = 0;
	return -1;
}

int rtl_http_hdr_to_string(char *str, size_t size, const rtl_http_hdr_list_t *list)
{
	int len = 0, i;

	if (!list || !str || size == 0)
		return -1;

	str[0] = '\0';

	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (list->header[i] && list->value[i]) {
			len = strlen(str);
			if (size - len > 0)
				snprintf(str + len, size - len, "%s: %s\r\n", list->header[i], list->value[i]);
			else
				return -1;
		}
	}

	return 0;
}

int rtl_http_hdr_clear_value(rtl_http_hdr_list_t *list, const char *name)
{
	int i;

	if ((list == NULL) || (name == NULL))
		return -1;
	for (i = 0; i < RTL_HTTP_HDRS_MAX; i++) {
		if (name && list->header[i] &&
			(strcasecmp(list->header[i], name) == 0)) {
			if (rtl_http_hdr_is_known(name) == NULL)
				free(list->header[i]);
			list->header[i] = NULL;
			free(list->value[i]);
			list->value[i] = NULL;
		}
	}
	return 0;
}
