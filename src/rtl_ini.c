#include <ctype.h>
#include <stdarg.h>

#include "rtl_ini.h"

#define ASCIILINESZ         (1024)
#define INI_INVALID_KEY     ((char*)-1)

/* This enum stores the status for each parsed line (internal use only). */
typedef enum _line_status_ {
	LINE_UNPROCESSED,
	LINE_ERROR,
	LINE_EMPTY,
	LINE_COMMENT,
	LINE_SECTION,
	LINE_VALUE
} line_status;

/**
 * @brief    Convert a string to lowercase.
 * @param    in   String to convert.
 * @param    out Output buffer.
 * @param    len Size of the out buffer.
 * @return   ptr to the out buffer or NULL if an error occured.
 *
 * This function convert a string into lowercase.
 * At most len - 1 elements of the input string will be converted.
 */
static const char *strlwc(const char *in, char *out, unsigned len)
{
	unsigned i;

	if (in == NULL || out == NULL || len == 0)
		return NULL;
	i = 0;
	while (in[i] != '\0' && i < len - 1) {
		out[i] = (char)tolower((int)in[i]);
		i++;
	}
	out[i] = '\0';
	return out;
}

/**
 * @brief    Remove blanks at the beginning and the end of a string.
 * @param    str  String to parse and alter.
 * @return   unsigned New size of the string.
 */
static unsigned strstrip(char *s)
{
	char *last = NULL;
	char *dest = s;

	if (s == NULL)
		return 0;

	last = s + strlen(s);
	while (isspace((int)*s) && *s)
		s++;
	while (last > s) {
		if (!isspace((int)*(last - 1)))
			break;
		last--;
	}
	*last = (char)0;

	memmove(dest, s, last - s + 1);
	return last - s;
}

/**
 * @brief    Default error callback for ini: wraps `fprintf(stderr, ...)`.
 */
static int default_error_callback(const char *format, ...)
{
	int ret;
	va_list argptr;

	va_start(argptr, format);
	ret = vfprintf(stderr, format, argptr);
	va_end(argptr);
	return ret;
}

static int (*rtl_ini_error_callback) (const char *, ...) =
	default_error_callback;

void rtl_ini_set_error_callback(int (*errback) (const char *, ...))
{
	if (errback) {
		rtl_ini_error_callback = errback;
	} else {
		rtl_ini_error_callback = default_error_callback;
	}
}

int rtl_ini_get_nsec(const rtl_dict_t *d)
{
	int i;
	int nsec;

	if (d == NULL)
		return -1;
	nsec = 0;
	for (i = 0; i < d->size; i++) {
		if (d->key[i] == NULL)
			continue;
		if (strchr(d->key[i], ':') == NULL) {
			nsec++;
		}
	}
	return nsec;
}

const char *rtl_ini_get_sec_name(const rtl_dict_t *d, int n)
{
	int i;
	int foundsec;

	if (d == NULL || n < 0)
		return NULL;
	foundsec = 0;
	for (i = 0; i < d->size; i++) {
		if (d->key[i] == NULL)
			continue;
		if (strchr(d->key[i], ':') == NULL) {
			foundsec++;
			if (foundsec > n)
				break;
		}
	}
	if (foundsec <= n) {
		return NULL;
	}
	return d->key[i];
}

void rtl_ini_dump(const rtl_dict_t *d, FILE *f)
{
	int i;

	if (d == NULL || f == NULL)
		return;
	for (i = 0; i < d->size; i++) {
		if (d->key[i] == NULL)
			continue;
		if (d->val[i] != NULL) {
			fprintf(f, "[%s]=[%s]\n", d->key[i], d->val[i]);
		} else {
			fprintf(f, "[%s]=UNDEF\n", d->key[i]);
		}
	}
	return;
}

void rtl_ini_dump_ini(const rtl_dict_t *d, FILE *f)
{
	int i;
	int nsec;
	const char *secname;

	if (d == NULL || f == NULL)
		return;

	nsec = rtl_ini_get_nsec(d);
	if (nsec < 1) {
		/* No section in file: dump all keys as they are */
		for (i = 0; i < d->size; i++) {
			if (d->key[i] == NULL)
				continue;
			fprintf(f, "%s = %s\n", d->key[i], d->val[i]);
		}
		return;
	}
	for (i = 0; i < nsec; i++) {
		secname = rtl_ini_get_sec_name(d, i);
		rtl_ini_dump_section_ini(d, secname, f);
	}
	return;
}

void rtl_ini_dump_section_ini(const rtl_dict_t *d, const char *s, FILE *f)
{
	int j;
	char keym[ASCIILINESZ + 1];
	int seclen;

	if (d == NULL || f == NULL)
		return;
	if (!rtl_ini_find_entry(d, s))
		return;

	seclen = (int)strlen(s);
	fprintf(f, "[%s]\n", s);
	sprintf(keym, "%s:", s);
	for (j = 0; j < d->size; j++) {
		if (d->key[j] == NULL)
			continue;
		if (!strncmp(d->key[j], keym, seclen + 1)) {
			fprintf(f, "\t%-16s = %s\n",
					d->key[j] + seclen + 1, d->val[j] ? d->val[j] : "");
		}
	}
	fprintf(f, "\n");
	return;
}

int rtl_ini_get_sec_nkeys(const rtl_dict_t *d, const char *s)
{
	int seclen, nkeys;
	char keym[ASCIILINESZ + 1];
	int j;

	nkeys = 0;

	if (d == NULL)
		return nkeys;
	if (!rtl_ini_find_entry(d, s))
		return nkeys;

	seclen = (int)strlen(s);
	strlwc(s, keym, sizeof(keym));
	keym[seclen] = ':';

	for (j = 0; j < d->size; j++) {
		if (d->key[j] == NULL)
			continue;
		if (!strncmp(d->key[j], keym, seclen + 1))
			nkeys++;
	}

	return nkeys;

}

const char **rtl_ini_get_sec_keys(const rtl_dict_t *d, const char *s,
								  const char **keys)
{
	int i, j, seclen;
	char keym[ASCIILINESZ + 1];

	if (d == NULL || keys == NULL)
		return NULL;
	if (!rtl_ini_find_entry(d, s))
		return NULL;

	seclen = (int)strlen(s);
	strlwc(s, keym, sizeof(keym));
	keym[seclen] = ':';

	i = 0;

	for (j = 0; j < d->size; j++) {
		if (d->key[j] == NULL)
			continue;
		if (!strncmp(d->key[j], keym, seclen + 1)) {
			keys[i] = d->key[j];
			i++;
		}
	}

	return keys;
}

const char *rtl_ini_get_string(const rtl_dict_t *d, const char *key,
							   const char *def)
{
	const char *lc_key;
	const char *sval;
	char tmp_str[ASCIILINESZ + 1];

	if (d == NULL || key == NULL)
		return def;

	lc_key = strlwc(key, tmp_str, sizeof(tmp_str));
	sval = rtl_dict_get(d, lc_key, def);
	return sval;
}

long int rtl_ini_get_longint(const rtl_dict_t *d, const char *key,
							 long int notfound)
{
	const char *str;

	str = rtl_ini_get_string(d, key, INI_INVALID_KEY);
	if (str == INI_INVALID_KEY)
		return notfound;
	return strtol(str, NULL, 0);
}

int rtl_ini_get_int(const rtl_dict_t *d, const char *key, int notfound)
{
	return (int)rtl_ini_get_longint(d, key, notfound);
}

double rtl_ini_get_double(const rtl_dict_t *d, const char *key,
						  double notfound)
{
	const char *str;

	str = rtl_ini_get_string(d, key, INI_INVALID_KEY);
	if (str == INI_INVALID_KEY)
		return notfound;
	return atof(str);
}

int rtl_ini_get_boolean(const rtl_dict_t *d, const char *key, int notfound)
{
	int ret;
	const char *c;

	c = rtl_ini_get_string(d, key, INI_INVALID_KEY);
	if (c == INI_INVALID_KEY)
		return notfound;
	if (c[0] == 'y' || c[0] == 'Y' || c[0] == '1' || c[0] == 't' || c[0] == 'T') {
		ret = 1;
	} else if (c[0] == 'n' || c[0] == 'N' || c[0] == '0' || c[0] == 'f'
			   || c[0] == 'F') {
		ret = 0;
	} else {
		ret = notfound;
	}
	return ret;
}

int rtl_ini_find_entry(const rtl_dict_t *ini, const char *entry)
{
	int found = 0;

	if (rtl_ini_get_string(ini, entry, INI_INVALID_KEY) != INI_INVALID_KEY) {
		found = 1;
	}
	return found;
}

int rtl_ini_set(rtl_dict_t *ini, const char *entry, const char *val)
{
	char tmp_str[ASCIILINESZ + 1];

	return rtl_dict_set(ini, strlwc(entry, tmp_str, sizeof(tmp_str)), val);
}

void rtl_ini_unset(rtl_dict_t *ini, const char *entry)
{
	char tmp_str[ASCIILINESZ + 1];

	rtl_dict_unset(ini, strlwc(entry, tmp_str, sizeof(tmp_str)));
}

/**
 * @brief    Load a single line from an INI file
 * @param    input_line  Input line, may be concatenated multi-line input
 * @param    section     Output space to store section
 * @param    key         Output space to store key
 * @param    value       Output space to store value
 * @return   line_status value
 */
static line_status ini_line(const char *input_line, char *section,
							char *key, char *value)
{
	line_status sta;
	char *line = NULL;
	size_t len;

	line = strdup(input_line);
	len = strstrip(line);

	sta = LINE_UNPROCESSED;
	if (len < 1) {
		/* Empty line */
		sta = LINE_EMPTY;
	} else if (line[0] == '#' || line[0] == ';') {
		/* Comment line */
		sta = LINE_COMMENT;
	} else if (line[0] == '[' && line[len - 1] == ']') {
		/* Section name */
		sscanf(line, "[%[^]]", section);
		strstrip(section);
		strlwc(section, section, len);
		sta = LINE_SECTION;
	} else if (sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2
			   || sscanf(line, "%[^=] = '%[^\']'", key, value) == 2) {
		/* Usual key=value with quotes, with or without comments */
		strstrip(key);
		strlwc(key, key, len);
		/* Don't strip spaces from values surrounded with quotes */
		sta = LINE_VALUE;
	} else if (sscanf(line, "%[^=] = %[^;#]", key, value) == 2) {
		/* Usual key=value without quotes, with or without comments */
		strstrip(key);
		strlwc(key, key, len);
		strstrip(value);
		/*
		 * sscanf cannot handle '' or "" as empty values
		 * this is done here
		 */
		if (!strcmp(value, "\"\"") || (!strcmp(value, "''"))) {
			value[0] = 0;
		}
		sta = LINE_VALUE;
	} else if (sscanf(line, "%[^=] = %[;#]", key, value) == 2
			   || sscanf(line, "%[^=] %[=]", key, value) == 2) {
		/*
		 * Special cases:
		 * key=
		 * key=;
		 * key=#
		 */
		strstrip(key);
		strlwc(key, key, len);
		value[0] = 0;
		sta = LINE_VALUE;
	} else {
		/* Generate syntax error */
		sta = LINE_ERROR;
	}

	free(line);
	return sta;
}

rtl_dict_t *rtl_ini_load(const char *ininame)
{
	FILE *in;

	char line[ASCIILINESZ + 1];
	char section[ASCIILINESZ + 1];
	char key[ASCIILINESZ + 1];
	char tmp[(ASCIILINESZ *2) + 1];
	char val[ASCIILINESZ + 1];

	int last = 0;
	int len;
	int lineno = 0;
	int errs = 0;
	int mem_err = 0;

	rtl_dict_t *dict;

	if ((in = fopen(ininame, "r")) == NULL) {
		rtl_ini_error_callback("ini: cannot open %s\n", ininame);
		return NULL;
	}

	dict = rtl_dict_new(0);
	if (!dict) {
		fclose(in);
		return NULL;
	}

	memset(line, 0, ASCIILINESZ);
	memset(section, 0, ASCIILINESZ);
	memset(key, 0, ASCIILINESZ);
	memset(val, 0, ASCIILINESZ);
	last = 0;

	while (fgets(line + last, ASCIILINESZ - last, in) != NULL) {
		lineno++;
		len = (int)strlen(line) - 1;
		if (len <= 0)
			continue;
		/* Safety check against buffer overflows */
		if (line[len] != '\n' && !feof(in)) {
			rtl_ini_error_callback
				("ini: input line too long in %s (%d)\n", ininame,
				 lineno);
			rtl_dict_del(dict);
			fclose(in);
			return NULL;
		}
		/* Get rid of \n and spaces at end of line */
		while ((len >= 0) && ((line[len] == '\n') || (isspace(line[len])))) {
			line[len] = 0;
			len--;
		}
		if (len < 0) {			/* Line was entirely \n and/or spaces */
			len = 0;
		}
		/* Detect multi-line */
		if (line[len] == '\\') {
			/* Multi-line value */
			last = len;
			continue;
		} else {
			last = 0;
		}
		switch (ini_line(line, section, key, val)) {
		case LINE_EMPTY:
		case LINE_COMMENT:
			break;

		case LINE_SECTION:
			mem_err = rtl_dict_set(dict, section, NULL);
			break;

		case LINE_VALUE:
			sprintf(tmp, "%s:%s", section, key);
			mem_err = rtl_dict_set(dict, tmp, val);
			break;

		case LINE_ERROR:
			rtl_ini_error_callback
				("ini: syntax error in %s (%d):\n-> %s\n", ininame,
				 lineno, line);
			errs++;
			break;

		default:
			break;
		}
		memset(line, 0, ASCIILINESZ);
		last = 0;
		if (mem_err < 0) {
			rtl_ini_error_callback("ini: memory allocation failure\n");
			break;
		}
	}
	if (errs) {
		rtl_dict_del(dict);
		dict = NULL;
	}
	fclose(in);
	return dict;
}

void rtl_ini_free_dict(rtl_dict_t *d)
{
	rtl_dict_del(d);
}
