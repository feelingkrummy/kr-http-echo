#ifndef _HTTP_H_
#define _HTTP_H_

#include "string.h"

#define HTTP_BUF_SIZE 4096

struct string8 read_http_request(int fd);

#endif;
