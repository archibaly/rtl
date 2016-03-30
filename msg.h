#ifndef _MSG_H_
#define _MSG_H_

#include <stdint.h>

#define MSG_DATA_TYPE_STRING	1
#define MSG_DATA_TYPE_UINT8		2
#define MSG_DATA_TYPE_UINT16	3
#define MSG_DATA_TYPE_UINT32	4
#define MSG_DATA_TYPE_INT8		5
#define MSG_DATA_TYPE_INT16		6
#define MSG_DATA_TYPE_INT32		7

#define MSG_VALID_CODE			0xaa

#define MSG_NO_ENCRYPTION		0
#define MSG_TEA_ENCRYPTION		1

#define MSG_MAX_PAYLOAD			2048	/* bytes */

typedef struct {
	uint8_t code;	/* 0xaa */
	uint8_t ecp;	/* 0: no encryption; 1: TEA encryption */
	uint16_t fun;	/* function of message, defined by user */
	int32_t len;	/* the length of whole message */
} msg_head;

typedef struct {
	msg_head head;
	uint8_t data[MSG_MAX_PAYLOAD];
} msg;

int payload_tea_encrypt(uint8_t *payload, int32_t len);
int payload_tea_decrypt(uint8_t *payload, int32_t len);
int msg_pack(msg *msg, uint8_t ecp, uint16_t fun, int sum, ...);
int msg_unpack(msg *msg, int sum, ...);
int msg_write(int fd, const msg *msg);
int msg_read(int fd, msg *msg);

#endif /* _MSG_H_ */
