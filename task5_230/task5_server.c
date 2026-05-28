/* --------------------------------------------------------------------------------------

 Project:       PG3401 Exam Task 5b
 Modified by:   Candidate 230
 Description:   TCP server speaking the EWP binary protocol (incremental build).

 Usage:         ./task5 -port <port> -id <id>

---------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "include/server.h"

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/

/* TCP is a stream protocol with no message boundaries, recv() may return
 * fewer bytes than requested. Loop until exactly iLen bytes are received. */
static int recvAll(int iFd, char *pBuf, int iLen) {
   int iTotal = 0;
   int iReceived = 0;
   while (iTotal < iLen) {
      iReceived = recv(iFd, pBuf + iTotal, iLen - iTotal, 0);
      if (iReceived <= 0) return -1;
      iTotal += iReceived;
   }
   return iTotal;
}

/* Parses the 4-digit ASCII size field from an EWP header into an integer. */
static int parseSize(const char *pszSize) {
   char szBuf[5] = {0};
   memcpy(szBuf, pszSize, 4);
   return atoi(szBuf);
}

/* Every outgoing EWP message starts with the same 8-byte header:
     "EWP" (3 bytes) identifier
     "0064" (4 bytes) total message size as 4-digit ASCII
     "|" (1 byte)  delimiter
   This helper fills that header into any EWP struct that starts with
   an EWA_EXAM25_TASK5_PROTOCOL_SIZEHEADER. */
static void fillHeader(struct EWA_EXAM25_TASK5_PROTOCOL_SIZEHEADER *pHead, int iTotalSize) {
   char szSize[5] = {0};
   memcpy(pHead->acMagicNumber, "EWP", 3);
   snprintf(szSize, sizeof(szSize), "%04d", iTotalSize);
   memcpy(pHead->acDataSize, szSize, 4);
   pHead->acDelimeter[0] = '|';
}

/* First message sent after a client connects.
   Format: "220 <ip> SMTP <serverId> <timestamp>"
   The 220 status code signals the server is ready to accept commands. */
static int sendServerAccept(int iSockFd, const char *pszId) {
   struct EWA_EXAM25_TASK5_PROTOCOL_SERVERACCEPT stMsg;
   time_t tNow = 0;
   struct tm *pTm = NULL;
   char szTimestamp[32] = {0};

   memset(&stMsg, 0, sizeof(stMsg));
   fillHeader(&stMsg.stHead, FIXED_MSG_SIZE);

   memcpy(stMsg.acStatusCode, "220", 3);
   stMsg.acHardSpace[0] = ' ';

   time(&tNow);
   pTm = localtime(&tNow);
   strftime(szTimestamp, sizeof(szTimestamp), "%d %b %Y, %H:%M:%S", pTm);

   snprintf(stMsg.acFormattedString, sizeof(stMsg.acFormattedString),
            "127.0.0.1 SMTP %s %s", pszId, szTimestamp);

   /* acHardZero already zero from memset */

   printf("-> 220 (SERVERACCEPT)\n");
   return send(iSockFd, &stMsg, sizeof(stMsg), 0);
}

/* Reads the HELO message sent by EWA after it receives our 220.
   Extracts the username+IP string EWA identified itself with.
   Returns 0 on success, -1 on recv failure. */
static int recvClientHelo(int iSockFd, char *pszClientId, int iIdBufLen) {
   struct EWA_EXAM25_TASK5_PROTOCOL_CLIENTHELO stMsg;

   memset(&stMsg, 0, sizeof(stMsg));

   if (recvAll(iSockFd, (char *)&stMsg, FIXED_MSG_SIZE) < 0) {
      printf("ERROR: lost connection reading HELO\n");
      return -1;
   }

   strncpy(pszClientId, stMsg.acFormattedString, iIdBufLen - 1);
   pszClientId[iIdBufLen - 1] = '\0';

   printf("<- %.4s %s\n", stMsg.acCommand, pszClientId);
   return 0;
}

/* Replies to HELO with a 250 greeting.
   Splits pszClientId ("username.ip") on the first dot to produce
   the response format: "250 <ip> Hello <username>" */
static int sendServerHelo(int iSockFd, const char *pszClientId) {
   struct EWA_EXAM25_TASK5_PROTOCOL_SERVERHELO stMsg;
   char szUser[28] = {0};
   char szIp[16] = {0};
   char *pDot = NULL;

   memset(&stMsg, 0, sizeof(stMsg));
   fillHeader(&stMsg.stHead, FIXED_MSG_SIZE);

   memcpy(stMsg.acStatusCode, "250", 3);
   stMsg.acHardSpace[0] = ' ';

   /* Split "username.ip" on the first dot */
   pDot = strchr(pszClientId, '.');
   if (pDot != NULL) {
      int iUserLen = (int)(pDot - pszClientId);
      strncpy(szUser, pszClientId, iUserLen);
      strncpy(szIp, pDot + 1, sizeof(szIp) - 1);
   } else {
      strncpy(szUser, pszClientId, sizeof(szUser) - 1);
      strncpy(szIp, "unknown", sizeof(szIp) - 1);
   }

   snprintf(stMsg.acFormattedString, sizeof(stMsg.acFormattedString),
            "%s Hello %s", szIp, szUser);

   printf("-> 250 (SERVERHELO) %s Hello %s\n", szIp, szUser);
   return send(iSockFd, &stMsg, sizeof(stMsg), 0);
}

/* Reads the MAILFROM message sent by EWA after it receives our 250.
   Extracts the sender address EWA identified itself with.
   Returns 0 on success, -1 on recv failure. */
static int recvClientMailFrom(int iSockFd, char *pszSender, int iSenderBufLen) {
   struct EWA_EXAM25_TASK5_PROTOCOL_MAILFROM stMsg;

   memset(&stMsg, 0, sizeof(stMsg));

   if (recvAll(iSockFd, (char *)&stMsg, FIXED_MSG_SIZE) < 0) {
      printf("ERROR: lost connection reading MAILFROM\n");
      return -1;
   }

   strncpy(pszSender, stMsg.acFormattedString, iSenderBufLen - 1);
   pszSender[iSenderBufLen - 1] = '\0';

   printf("<- %.10s %s\n", stMsg.acCommand, pszSender);
   return 0;
}

/* Generic status reply used for 250 (ok), 354 (ready for data),
   501 (bad filename), and 221 (closing).
   pszMsg is the human-readable text placed in acFormattedString. */
static int sendServerReply(int iSockFd, const char *pszCode, const char *pszMsg) {
   struct EWA_EXAM25_TASK5_PROTOCOL_SERVERREPLY stMsg;

   memset(&stMsg, 0, sizeof(stMsg));
   fillHeader(&stMsg.stHead, FIXED_MSG_SIZE);

   memcpy(stMsg.acStatusCode, pszCode, 3);
   stMsg.acHardSpace[0] = ' ';
   strncpy(stMsg.acFormattedString, pszMsg, sizeof(stMsg.acFormattedString) - 1);

   printf("-> %s %s\n", pszCode, pszMsg);
   return send(iSockFd, &stMsg, sizeof(stMsg), 0);
}

/* Reads the RCPTTO message sent by EWA after it receives our 250.
   Extracts the recipient address.
   Returns 0 on success, -1 on recv failure. */
static int recvClientRcptTo(int iSockFd, char *pszRecipient, int iRecipientBufLen) {
   struct EWA_EXAM25_TASK5_PROTOCOL_RCPTTO stMsg;

   memset(&stMsg, 0, sizeof(stMsg));

   if (recvAll(iSockFd, (char *)&stMsg, FIXED_MSG_SIZE) < 0) {
      printf("ERROR: lost connection reading RCPTTO\n");
      return -1;
   }

   strncpy(pszRecipient, stMsg.acFormattedString, iRecipientBufLen - 1);
   pszRecipient[iRecipientBufLen - 1] = '\0';

   printf("<- %.8s %s\n", stMsg.acCommand, pszRecipient);
   return 0;
}

/* Validates a filename against an allowlist of safe characters.
   Rejects empty names, names starting with '.', and any character
   outside a-z, A-Z, 0-9, '.', '-', '_'.
   Returns 1 if valid, 0 if not. */
static int validateFilename(const char *pszFilename) {
   int i = 0;

   if (pszFilename == NULL || pszFilename[0] == '\0') return 0;
   if (pszFilename[0] == '.') return 0;

   for (i = 0; pszFilename[i] != '\0'; i++) {
      char c = pszFilename[i];
      if ((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
           c == '.' || c == '-'  || c == '_') {
         continue;
      }
      return 0;
   }
   return 1;
}

/* Receives one or more CLIENTDATAFILE packets and accumulates the content
   into pBuf. Each packet has a dynamic 8-byte header whose acDataSize field
   (4-digit ASCII) gives the total packet size including the header.
   Content length = totalSize - HEADER_SIZE - 1 (the -1 strips the null terminator
   byte that follows the content, as noted in ewpdef.h).
   Loops until the END_MARKER is found, then strips it.
   Sets *piFileLen to the number of bytes written. Returns 0 on success, -1 on error. */
static int recvFileData(int iSockFd, char *pBuf, int iBufLen, int *piFileLen) {
   struct EWA_EXAM25_TASK5_PROTOCOL_SIZEHEADER stHead;
   char cNull = 0;
   int iMsgSize = 0;
   int iContentLen = 0;
   int iTotal = 0;

   while (1) {
      /* Read the 8-byte header */
      if (recvAll(iSockFd, (char *)&stHead, HEADER_SIZE) < 0) {
         printf("ERROR: lost connection reading file data header\n");
         return -1;
      }

      /* Validate magic: if header does not start with EWP, stop receiving */
      if (memcmp(stHead.acMagicNumber, "EWP", 3) != 0) break;

      /* acDataSize covers only the payload after the header (content + null byte).
         See ewpdef.h: "acFileContent will be stHead.acDataSize - 1 bytes".
         So content length = acDataSize - 1 (strip null). */
      iMsgSize    = parseSize(stHead.acDataSize);
      iContentLen = iMsgSize - 1;

      if (iContentLen <= 0 || iTotal + iContentLen > iBufLen) {
         printf("ERROR: content size %d out of range\n", iContentLen);
         return -1;
      }

      /* Read content, then consume the null terminator separately */
      if (recvAll(iSockFd, pBuf + iTotal, iContentLen) < 0) {
         printf("ERROR: lost connection reading file content\n");
         return -1;
      }
      recvAll(iSockFd, &cNull, 1);
      iTotal += iContentLen;

      /* Check if the end marker has arrived */
      if (iTotal >= END_MARKER_LEN &&
          memcmp(pBuf + iTotal - END_MARKER_LEN, END_MARKER, END_MARKER_LEN) == 0) {
         iTotal -= END_MARKER_LEN;
         break;
      }
   }

   *piFileLen = iTotal;
   printf("  [received %d bytes]\n", iTotal);
   return 0;
}

/* Writes the accumulated file buffer to disk using the validated filename.
   Returns 0 on success, -1 on error. */
static int saveFile(const char *pszFilename, const char *pBuf, int iLen) {
   FILE *pFile = NULL;

   pFile = fopen(pszFilename, "wb");
   if (pFile == NULL) {
      printf("ERROR: could not open '%s' for writing: %s\n", pszFilename, strerror(errno));
      return -1;
   }

   fwrite(pBuf, 1, iLen, pFile);
   fclose(pFile);
   printf("File saved: %s (%d bytes)\n", pszFilename, iLen);
   return 0;
}

static int initEwa(int iPort) {
   int iSockFd = -1;
   int iClientFd = -1;
   int iAddrLen = 0;
   struct sockaddr_in saAddr = {0};
   struct sockaddr_in saClient = {0};

   iSockFd = socket(AF_INET, SOCK_STREAM, 0);
   if (iSockFd < 0) {
      printf("ERROR: socket failed: %s\n", strerror(errno));
      return -1;
   }

   saAddr.sin_family = AF_INET;
   saAddr.sin_port = htons(iPort);
   saAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

   if (bind(iSockFd, (struct sockaddr *)&saAddr, sizeof(saAddr)) < 0) {
      if (errno == EACCES) {
         printf("ERROR: port %d requires root -- use a port above 1024\n", iPort);
      } else {
         printf("ERROR: bind failed: %s\n", strerror(errno));
      }
      close(iSockFd);
      return -1;
   }

   if (listen(iSockFd, BACKLOG) < 0) {
      printf("ERROR: listen failed: %s\n", strerror(errno));
      close(iSockFd);
      return -1;
   }

   printf("Server listening on port %d\n", iPort);

   iAddrLen  = sizeof(saClient);
   iClientFd = accept(iSockFd,
                      (struct sockaddr *)&saClient,
                      (socklen_t *)&iAddrLen);
   if (iClientFd < 0) {
      printf("ERROR: accept failed: %s\n", strerror(errno));
      close(iSockFd);
      return -1;
   }

   printf("Client connected from %s\n", inet_ntoa(saClient.sin_addr));
   close(iSockFd);
   return iClientFd;
}

static int runSmtpServer(int iSockFd, const char *pszId) {
   char szClientId[64] = {0};
   char szFilename[64] = {0};
   char *pFileBuf = NULL;
   int iFileLen = 0;
   char acCmd[FIXED_MSG_SIZE] = {0};
   struct EWA_EXAM25_TASK5_PROTOCOL_CLIENTDATACMD *pDataCmd = NULL;

   pFileBuf = (char *)malloc(MAX_FILE_BUF);
   if (pFileBuf == NULL) {
      printf("ERROR: malloc failed\n");
      return -1;
   }
   memset(pFileBuf, 0, MAX_FILE_BUF);

   /* Handshake */
   if (sendServerAccept(iSockFd, pszId) < 0) {
      printf("ERROR: failed to send SERVERACCEPT\n");
      free(pFileBuf);
      return -1;
   }

   if (recvClientHelo(iSockFd, szClientId, sizeof(szClientId)) < 0) {
      printf("ERROR: failed to receive CLIENTHELO\n");
      free(pFileBuf);
      return -1;
   }

   if (sendServerHelo(iSockFd, szClientId) < 0) {
      printf("ERROR: failed to send SERVERHELO\n");
      free(pFileBuf);
      return -1;
   }
   
   /* EWA sends mail */
   if (recvClientMailFrom(iSockFd, szClientId, sizeof(szClientId)) < 0) {
      printf("ERROR: failed to receive MAILFROM\n");
      free(pFileBuf);
      return -1;
   }

   if (sendServerReply(iSockFd, "250", "Sender address ok") < 0) {
      printf("ERROR: failed to send reply\n");
      free(pFileBuf);
      return -1;
   }

   if (recvClientRcptTo(iSockFd, szClientId, sizeof(szClientId)) < 0) {
      printf("ERROR: failed to receive RCPTTO\n");
      free(pFileBuf);
      return -1;
   }

   if (sendServerReply(iSockFd, "250", "Recipient address ok") < 0) {
      printf("ERROR: failed to send reply\n");
      free(pFileBuf);
      return -1;
   }

   /* DATA / QUIT loop: EWA sends multiple files and sends quit signal */
   while (1) {
      memset(acCmd, 0, sizeof(acCmd));

      if (recvAll(iSockFd, acCmd, FIXED_MSG_SIZE) < 0) {
         printf("ERROR: lost connection waiting for DATA/QUIT\n");
         free(pFileBuf);
         return -1;
      }

      /* Check command field, which sits immediately after the 8-byte header */
      if (memcmp(acCmd + HEADER_SIZE, "QUIT", 4) == 0) {
         printf("<- QUIT\n");
         sendServerReply(iSockFd, "221", "Bye");
         break;
      }

      if (memcmp(acCmd + HEADER_SIZE, "DATA", 4) == 0) {
         /* Cast the raw buffer to the DATA struct to extract the filename */
         pDataCmd = (struct EWA_EXAM25_TASK5_PROTOCOL_CLIENTDATACMD *)acCmd;
         memset(szFilename, 0, sizeof(szFilename));
         strncpy(szFilename, pDataCmd->acFormattedString, sizeof(szFilename) - 1);
         printf("<- DATA %s\n", szFilename);

         if (!validateFilename(szFilename)) {
            printf("ERROR: invalid filename '%s'\n", szFilename);
            sendServerReply(iSockFd, "501", "Invalid filename");
            continue;
         }

         if (sendServerReply(iSockFd, "354", "Ready for message") < 0) {
            printf("ERROR: failed to send reply\n");
            free(pFileBuf);
            return -1;
         }

         memset(pFileBuf, 0, MAX_FILE_BUF);
         iFileLen = 0;

         if (recvFileData(iSockFd, pFileBuf, MAX_FILE_BUF, &iFileLen) < 0) {
            free(pFileBuf);
            return -1;
         }

         saveFile(szFilename, pFileBuf, iFileLen);
         printf("--- File contents ---\n%.*s\n---------------------\n", iFileLen, pFileBuf);
         sendServerReply(iSockFd, "250", "Message accepted");
         continue;
      }
      printf("WARNING: unknown command, ignoring\n");
   }

   free(pFileBuf);
   return 0;
}

/* ------------------------------------------------------------------------------------*/
/* Main                                                                                */
/* ------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
   int iPort = 0;
   char szServerId[64] = {0};
   int iClientFd = 0;
   int i = 0;

   for (i = 1; i < argc - 1; i++) {
      if (strcmp(argv[i], "-port") == 0) {
         iPort = atoi(argv[i + 1]);
      } else if (strcmp(argv[i], "-id") == 0) {
         strncpy(szServerId, argv[i + 1], sizeof(szServerId) - 1);
      }
   }

   if (iPort == 0 || szServerId[0] == '\0') {
      fprintf(stderr, "Usage: %s -port <port> -id <id>\n", argv[0]);
      return EXIT_FAILURE;
   }

   printf("Starting server on port %d with id '%s'\n", iPort, szServerId);

   iClientFd = initEwa(iPort);
   if (iClientFd < 0) return EXIT_FAILURE;

   runSmtpServer(iClientFd, szServerId);

   close(iClientFd);
   return EXIT_SUCCESS;
}

