#include <stdio.h>
#include <string.h>

#include <rtl_sha1.h>

int sha1_test()
{
	char text1[] = {"abc"};
	char text2[] = {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	char text3[] = {"aaaaaaaaaa"};
	uint8_t hash1[RTL_SHA1_BLOCK_SIZE] = {0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d};
	uint8_t hash2[RTL_SHA1_BLOCK_SIZE] = {0x84,0x98,0x3e,0x44,0x1c,0x3b,0xd2,0x6e,0xba,0xae,0x4a,0xa1,0xf9,0x51,0x29,0xe5,0xe5,0x46,0x70,0xf1};
	uint8_t hash3[RTL_SHA1_BLOCK_SIZE] = {0x34,0xaa,0x97,0x3c,0xd4,0xc4,0xda,0xa4,0xf6,0x1e,0xeb,0x2b,0xdb,0xad,0x27,0x31,0x65,0x34,0x01,0x6f};
	uint8_t buf[RTL_SHA1_BLOCK_SIZE];
	int idx;
	rtl_sha1_ctx ctx;
	int pass = 1;

	rtl_sha1_init(&ctx);
	rtl_sha1_update(&ctx, (uint8_t *)text1, strlen(text1));
	rtl_sha1_final(&ctx, buf);
	pass = pass && !memcmp(hash1, buf, RTL_SHA1_BLOCK_SIZE);

	rtl_sha1_init(&ctx);
	rtl_sha1_update(&ctx, (uint8_t *)text2, strlen(text2));
	rtl_sha1_final(&ctx, buf);
	pass = pass && !memcmp(hash2, buf, RTL_SHA1_BLOCK_SIZE);

	rtl_sha1_init(&ctx);
	for (idx = 0; idx < 100000; ++idx)
	   rtl_sha1_update(&ctx, (uint8_t *)text3, strlen(text3));
	rtl_sha1_final(&ctx, buf);
	pass = pass && !memcmp(hash3, buf, RTL_SHA1_BLOCK_SIZE);

	return(pass);
}

int main()
{
	printf("SHA1 tests: %s\n", sha1_test() ? "SUCCEEDED" : "FAILED");

	char buf[RTL_SHA1_BLOCK_SIZE * 2 + 1];
	rtl_sha1_file("sha1.c", buf, sizeof(buf));
	printf("%s sha1.c\n", buf);

	return 0;
}
