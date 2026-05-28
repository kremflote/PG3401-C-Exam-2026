#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ewpdef.h"

#define BACKLOG 1
#define FIXED_MSG_SIZE 64
#define HEADER_SIZE 8         /* EWP (3) + size (4) + delimiter (1)     */
#define MAX_FILE_BUF 100000   /* Maximum accumulated file content       */
#define END_MARKER "\r\n\r\n.\r\n"
#define END_MARKER_LEN 7

