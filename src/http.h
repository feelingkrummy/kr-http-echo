#ifndef _HTTP_H_
#define _HTTP_H_

#include "string.h"

#define HTTP_BUF_SIZE 4096

extern const struct string8 not_found_msg;

struct string8 read_http_request(int fd);
int check_if_complete_request(struct string8 request);

#endif
