#!/bin/sh
SRC_FILE="\
	./src/string8.c \
	./src/tcp.c \
	./src/http.c \
	./src/main.c"

gcc $SRC_FILE -o kr-http
