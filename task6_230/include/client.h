#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ewpdef.h"

/* --- Protocol flag constants (from ewpdef.h comments) --- */
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_NACK 0x20   /* URG repurposed as NACK */

/* Header size = struct size minus the 1-byte Data[1] tail */
#define TCP_HDR_SIZE ((int)(sizeof(struct EWA_EXAM25_TASK4_PROTOCOL_TCP) - 1))

/* Out-of-order window slots */
#define MAX_OUT_OF_ORDER_SIZE 64

/* Max consecutive NACKs before accepting a packet regardless of checksum */
#define MAX_NACK_RETRIES 3

/* Max total file size to receive (10 MB) */
#define MAX_FILE_SIZE (10 * 1024 * 1024)

/* Output filename */
#define OUTPUT_FILE "received.bmp"

