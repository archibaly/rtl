#include <stdio.h>
#include <string.h>

#include <rtl_base64.h>

int base64_test()
{
	unsigned char text[3][1024] = {{"fo"},
	                      {"foobar"},
						  {"Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure."}};
	unsigned char code[3][1024] = {{"Zm8="},
	                      {"Zm9vYmFy"},
	                      {"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz\nIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg\ndGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu\ndWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo\nZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4="}};
	unsigned char buf[1024];
	size_t buf_len;
	int pass = 1;
	int idx;

	for (idx = 0; idx < 3; idx++) {
		buf_len = rtl_base64_encode(text[idx], buf, strlen((char *)text[idx]), 1);
		pass = pass && ((buf_len == strlen((char *)code[idx])) &&
		                 (buf_len == rtl_base64_encode(text[idx], NULL, strlen((char *)text[idx]), 1)));
		pass = pass && !strcmp((const char *)code[idx], (const char *)buf);

		memset(buf, 0, sizeof(buf));
		buf_len = rtl_base64_decode(code[idx], buf, strlen((char *)code[idx]));
		pass = pass && ((buf_len == strlen((char *)text[idx])) &&
		                (buf_len == rtl_base64_decode(code[idx], NULL, strlen((char *)code[idx]))));
		pass = pass && !strcmp((const char *)text[idx], (const char *)buf);
	}

	return pass;
}

int main()
{
	printf("Base64 tests: %s\n", base64_test() ? "PASSED" : "FAILED");

	return 0;
}
