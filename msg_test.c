#include <stdio.h>
#include <arpa/inet.h>

#include "msg.h"

int main()
{
	msg msg;
	char str[32];
	uint16_t u16;
	uint32_t u32;

	msg_pack(&msg,
			 0,
			 0,
			 3,
			 MSG_DATA_TYPE_STRING, "hello",
			 MSG_DATA_TYPE_UINT16, 1342,
			 MSG_DATA_TYPE_UINT32, 906821);

	printf("msg.head.code = %x\n", msg.head.code);
	printf("msg.head.ecp = %x\n", msg.head.ecp);
	printf("msg.head.fun = %x\n", msg.head.fun);
	printf("msg.head.len = %d\n", ntohl(msg.head.len));

	int i;
	for (i = 0; i < ntohl(msg.head.len) - sizeof(msg_head); i++)
		printf("%02x ", msg.data[i]);
	printf("\n");

	msg_unpack(&msg,
			   3,
			   MSG_DATA_TYPE_STRING, str,
			   MSG_DATA_TYPE_UINT16, &u16,
			   MSG_DATA_TYPE_UINT32, &u32);

	printf("str = %s\n", str);
	printf("u16 = %d\n", u16);
	printf("u32 = %d\n", u32);

	return 0;
}
