#ifndef _HTTPS_H_
#define _HTTPS_H_

#include <stdint.h>

int https_send_get(const char *host, uint16_t port, const char *path, char *resp, int len);

#endif /* _HTTPS_H_ */
