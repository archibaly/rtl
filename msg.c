#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "msg.h"

/*
 * ret = msg_pack(&msg, 0, 0,
 * 				  3,
 * 				  MSG_DATA_TYPE_STRING, "hello",
 * 				  MSG_DATA_TYPE_UINT16, 1001,
 * 				  MSG_DATA_TYPE_UINT32, 30217);
 */
int msg_pack(msg *msg, uint8_t ecp, uint16_t fun, int sum, ...)
{
	if (msg == NULL)
		return -1;

	msg->head.code = MSG_VALID_CODE;
	msg->head.ecp = ecp;
	msg->head.fun = htons(fun);

	int i;
	int type;
	int len;
	int offset = 0;
	va_list args;

	char *str;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	int8_t i8;
	int16_t i16;
	int32_t i32;

	va_start(args, sum);
	for (i = 0; i < sum; i++) {
		type = va_arg(args, int);
		switch (type) {
		case MSG_DATA_TYPE_STRING:
			str = va_arg(args, char *);
			len = strlen(str) + 1;
			memcpy(msg->data + offset, str, len);
			break;
		case MSG_DATA_TYPE_UINT8:
			u8 = (uint8_t)va_arg(args, int);
			len = sizeof(uint8_t);
			msg->data[offset] = u8;
			break;
		case MSG_DATA_TYPE_UINT16:
			u16 = htons((uint16_t)va_arg(args, int));
			len = sizeof(uint16_t);
			*(uint16_t *)&msg->data[offset] = u16;
			break;
		case MSG_DATA_TYPE_UINT32:
			u32 = htonl((uint32_t)va_arg(args, int));
			len = sizeof(uint32_t);
			*(uint32_t *)&msg->data[offset] = u32;
			break;
		case MSG_DATA_TYPE_INT8:
			i8 = (int8_t)va_arg(args, int);
			len = sizeof(int8_t);
			msg->data[offset] = i8;
			break;
		case MSG_DATA_TYPE_INT16:
			i16 = htons((int16_t)va_arg(args, int));
			len = sizeof(int16_t);
			*(int16_t *)&msg->data[offset] = i16;
			break;
		case MSG_DATA_TYPE_INT32:
			i32 = htonl((int32_t)va_arg(args, int));
			len = sizeof(int32_t);
			*(int32_t *)&msg->data[offset] = i32;
			break;
		default:
			len = 0;
			break;
		}
		offset += len;
	}
	msg->head.len = htonl(offset + sizeof(msg_head));

	va_end(args);
	return 0;
}

/*
 * ret = msg_unpack(&msg,
 * 					2,
 * 					MSG_DATA_TYPE_STRING, sizeof(str), str,
 * 					MSG_DATA_TYPE_UINT32, sizeof(uint32_t), i);
 */
int msg_unpack(msg *msg, int sum, ...)
{
	if (msg == NULL)
		return -1;

	int i;
	int type;
	va_list args;

	char *str;
	uint8_t *u8;
	uint16_t *u16;
	uint32_t *u32;
	int8_t *i8;
	int16_t *i16;
	int32_t *i32;

	int len;
	int offset = 0;

	va_start(args, sum);
	for (i = 0; i < sum; i++) {
		type = va_arg(args, int);
		switch (type) {
		case MSG_DATA_TYPE_STRING:
			str = va_arg(args, char *);
			len = strlen((char *)(msg->data + offset)) + 1;
			memcpy(str, msg->data + offset, len);
			break;
		case MSG_DATA_TYPE_UINT8:
			u8 = va_arg(args, uint8_t *);
			len = sizeof(uint8_t);
			*u8 = *((uint8_t *)(msg->data + offset));
			break;
		case MSG_DATA_TYPE_UINT16:
			u16 = va_arg(args, uint16_t *);
			len = sizeof(uint16_t);
			*u16 = ntohs(*((uint16_t *)(msg->data + offset)));
			break;
		case MSG_DATA_TYPE_UINT32:
			u32 = va_arg(args, uint32_t *);
			len = sizeof(uint32_t);
			*u32 = ntohl(*((uint32_t *)(msg->data + offset)));
			break;
		case MSG_DATA_TYPE_INT8:
			i8 = va_arg(args, int8_t *);
			len = sizeof(int8_t);
			*i8 = *((int8_t *)(msg->data + offset));
			break;
		case MSG_DATA_TYPE_INT16:
			i16 = va_arg(args, int16_t *);
			len = sizeof(int16_t);
			*i16 = ntohs(*((int16_t *)(msg->data + offset)));
			break;
		case MSG_DATA_TYPE_INT32:
			i32 = va_arg(args, int32_t *);
			len = sizeof(int32_t);
			*i32 = ntohl(*((int32_t *)(msg->data + offset)));
			break;
		default:
			len = 0;
			break;
		}
		offset += len;
	}
	va_end(args);

	return -1;
}
