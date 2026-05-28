/* --------------------------------------------------------------------------------------

 Project:       PG3401 Exam Task 6
 Modified by:   Candidate 230
 Description:   TCP client that connects to EWA's server and receives a BMP file
                using the custom "TCP over TCP" EWP protocol defined in ewpdef.h.

 Usage:         ./task6 -server 127.0.0.1 -port 42072

 Sources:       Lana Romero, F. J. (2014). CRC calculator [Source code]. SourceForge.
				             Retrieved May 5, 2026, from
				             https://sourceforge.net/projects/crccalculator/files/CRC/


---------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "include/client.h"
#include "include/crc.h"

/* ------------------------------------------------------------------------------------*/
/* Structs                                        */
/* ------------------------------------------------------------------------------------*/

/* Slot buffer for out-of-order packet storage */
typedef struct {
   int bUsed;
   int bIsFin;
   unsigned int uiSeqNum;
   unsigned short usDataSize;
   char *pData;
} Slot_t;

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/

/* Implementation: Francisco Javier Lana Romero's CRC library (crc.h & include/crc.c).
   F_CRC_InicializaTabla() must be called once before any checksum is
   computed (done at the top of main()).
   Found this implementation at https://en.wikipedia.org/wiki/Cyclic_redundancy_check
   The second link under "Implementation"
   However, it does not work. EWA has an implementation which I was unable to discover
   Therefore I accept any packets after 3 consecutive NACKs. Unsatisfactory but 
   the best I could accomplish. I think we just need to swap the CRC implementation with
   the correct one for our program to work perfectly. (except on the packages 
   EWA corrupts intentionally.)                                                        */
static unsigned short computeChecksum(const struct EWA_EXAM25_TASK4_PROTOCOL_TCP *pHdr,
                                      const char *pData, unsigned short usDataSize) {
   struct EWA_EXAM25_TASK4_PROTOCOL_TCP stTmp;
   unsigned int uiTotal = 0;
   char *pWork = NULL;
   crc usCrc = 0;

   /* Zero the checksum field before computing so it does not influence the result. */
   memcpy(&stTmp, pHdr, TCP_HDR_SIZE);
   stTmp.usChecksum = 0;

   /* Combine header + payload into a one buffer for checksum function to read */
   uiTotal = (unsigned int)(TCP_HDR_SIZE + usDataSize);
   pWork   = (char *)malloc(uiTotal + 1);
   if (pWork == NULL) return 0;
   memset(pWork, 0, uiTotal + 1); /* +1 just to be defensive */
   memcpy(pWork, &stTmp, TCP_HDR_SIZE);
   if (usDataSize > 0 && pData != NULL)
      memcpy(pWork + TCP_HDR_SIZE, pData, usDataSize);

   /* Here I use Romero's CRC implementation to calculate checksum */
   usCrc = F_CRC_CalculaCheckSum((uint8_t *)pWork, (uint16_t)uiTotal);
   free(pWork);
   return (unsigned short)usCrc; /* casting to be explicit  */
}

/* Loops until exactly iLen bytes have been received on iFd.
   Returns total bytes received, or -1 on connection loss. */
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

/* Builds and sends a header-only ACK or NACK packet to the server.
   ucFlags should be TCP_FLAG_ACK or TCP_FLAG_NACK.
   uiAckNum echoes back the sequence number of the packet being acknowledged. */
static int sendResponse(int iSockFd, unsigned int uiAckNum, unsigned char ucFlags) {
   struct EWA_EXAM25_TASK4_PROTOCOL_TCP stAck;
   memset(&stAck, 0, sizeof(stAck));
   stAck.uiAckNumber = uiAckNum;
   stAck.ucDataOffset = 5;
   stAck.ucFlags = ucFlags;
   stAck.usSizeOfPacket = 0;
   stAck.usChecksum = computeChecksum(&stAck, NULL, 0);
   printf("-> %s ack=%u checksum=0x%04x\n",
          (ucFlags & TCP_FLAG_ACK) ? "ACK" : "NACK",
          uiAckNum, stAck.usChecksum);
   return send(iSockFd, &stAck, TCP_HDR_SIZE, 0);
}

/* Writes iLen bytes from pBuf to OUTPUT_FILE on disk. */
static void saveFile(const char *pBuf, int iLen) {
   FILE *pFile = NULL;
   pFile = fopen(OUTPUT_FILE, "wb");

   if (pFile == NULL) {
      printf("ERROR: cannot open %s: %s\n", OUTPUT_FILE, strerror(errno));
      return;
   }

   fwrite(pBuf, 1, iLen, pFile);
   fclose(pFile);
   printf("File saved: %s (%d bytes)\n", OUTPUT_FILE, iLen);
}


/* Creates a TCP socket and connects to pszIp: iPort.
   Returns the socket file descriptor on success, -1 on failure. */
static int connectToServer(const char *pszIp, int iPort) {
   int iSockFd = -1;
   struct sockaddr_in saAddr = {0};

   iSockFd = socket(AF_INET, SOCK_STREAM, 0);
   if (iSockFd < 0) {
      printf("ERROR: socket failed: %s\n", strerror(errno));
      return -1;
   }

   saAddr.sin_family = AF_INET;
   saAddr.sin_port   = htons(iPort);
   if (inet_pton(AF_INET, pszIp, &saAddr.sin_addr) <= 0) {
      printf("ERROR: invalid address '%s'\n", pszIp);
      close(iSockFd);
      return -1;
   }

   if (connect(iSockFd, (struct sockaddr *)&saAddr, sizeof(saAddr)) < 0) {
      printf("ERROR: connect failed: %s\n", strerror(errno));
      close(iSockFd);
      return -1;
   }

   printf("Connected to %s:%d\n", pszIp, iPort);
   return iSockFd;
}


/* Receives all EWP packets from the server, reassembles them into the correct
   byte order, and saves the result to disk.
  
   Checksum: each packet is verified with computeChecksum(). A mismatch sends
   a NACK, which asks EWA to retransmit. 
  
   Out-of-order packets: uiSequenceNumber is a byte offset into the final file.
   Packets arriving out of order are held in aSlots[] and written to the file
   buffer once the preceding gap is filled. The drain loop runs after every 
   in-order arrival to write any newly contiguous slots
  
   FIN: the FIN flag marks the last packet EWA will send, but it may arrive
   before higher-offset out-of-order slots are drained. uiTotalBytes is
   therefore set after the drain loop, not when FIN is first seen. */
static void receiveFile(int iSockFd) {
   char *pFileBuf = NULL;           /* Heap buffer that accumulates the full file */
   char *pPacket = NULL;            /* Heap buffer for the current packet's payload */

   unsigned char  ucFlags = 0;      /* Flag byte from the packet header (ACK, FIN, etc.) */
   int bFinReceived = 0;            /* Flag: FIN packet has been seen */
   int bChecksumOk = 0;             /* Flag: checksum verified (or NACK limit reached) */
   int bFound = 0;                  /* Flag: a matching slot was found during drain loop */

   int i = 0;                       /* General loop counter */
   int iNackCount = 0;              /* How many NACKs sent for the current packet so far */

   unsigned int uiNextExpected = 0; /* Byte offset of the next in-order packet we expect */
   unsigned int uiTotalBytes = 0;   /* Final file size, set once FIN and all slots are drained */
   unsigned int uiSeqNum = 0;       /* Sequence number (byte offset) from the packet header */

   unsigned short usDataSize = 0;   /* Payload byte count from the packet header */
   unsigned short usRxCsum = 0;     /* Checksum received in the packet header */
   unsigned short usCalcCsum = 0;   /* Checksum we compute locally to verify against usRxCsum */

   Slot_t aSlots[MAX_OUT_OF_ORDER_SIZE]; /* Fixed-size buffer for out-of-order packets awaiting reassembly */
   struct EWA_EXAM25_TASK4_PROTOCOL_TCP stHdr; /* Parsed header of the current packet */

   pFileBuf = (char *)malloc(MAX_FILE_SIZE);
   if (pFileBuf == NULL) {
      printf("ERROR: malloc failed for file buffer\n");
      return;
   }
   memset(pFileBuf, 0, MAX_FILE_SIZE);
   memset(aSlots, 0, sizeof(aSlots));

   while (1) {
      /* Receive one packet, retrying up to MAX_NACK_RETRIES on bad CRC. */
      iNackCount  = 0;
      bChecksumOk = 0;

      do{
         if (recvAll(iSockFd, (char *)&stHdr, TCP_HDR_SIZE) < 0) {
            printf("ERROR: lost connection reading header\n");
            free(pFileBuf);
            return;
         }

         usDataSize = stHdr.usSizeOfPacket;
         uiSeqNum = stHdr.uiSequenceNumber;
         ucFlags = stHdr.ucFlags;
         usRxCsum = stHdr.usChecksum;

         if (pPacket != NULL) { free(pPacket); pPacket = NULL; }
         pPacket = (char *)malloc(usDataSize + 1);
         if (pPacket == NULL) {
            printf("ERROR: malloc failed for packet\n");
            free(pFileBuf);
            return;
         }
         memset(pPacket, 0, usDataSize + 1);

         if (usDataSize > 0) {
            if (recvAll(iSockFd, pPacket, usDataSize) < 0) {
               printf("ERROR: lost connection reading payload\n");
               free(pPacket);
               free(pFileBuf);
               return;
            }
         }

         usCalcCsum  = computeChecksum(&stHdr, pPacket, usDataSize);
         bChecksumOk = (usCalcCsum == usRxCsum);

         printf("<- seq=%-6u size=%-5u flags=0x%02x csum=0x%04x calc=0x%04x %s\n",
                 uiSeqNum, usDataSize, ucFlags,
                 usRxCsum, usCalcCsum,
                 bChecksumOk ? "OK" : "BAD");

         if (!bChecksumOk) {
            iNackCount++;
            if (iNackCount >= MAX_NACK_RETRIES) {
               printf("[checksum still bad after %d NACKs: accepting packet]\n",
                       MAX_NACK_RETRIES);
               bChecksumOk = 1;
            } else {
               printf("[NACK #%d -- requesting retransmit]\n", iNackCount);
               sendResponse(iSockFd, uiSeqNum, TCP_FLAG_NACK);
            }
         }

      } while (!bChecksumOk);

      sendResponse(iSockFd, uiSeqNum, TCP_FLAG_ACK);

      /* Reassembly */
      if (uiSeqNum == uiNextExpected) {

         if ((int)(uiSeqNum + usDataSize) <= MAX_FILE_SIZE)
            memcpy(pFileBuf + uiSeqNum, pPacket, usDataSize);
         uiNextExpected = uiSeqNum + usDataSize;

         if (ucFlags & TCP_FLAG_FIN)
            bFinReceived = 1;

         /* Drain buffered slots that are now contiguous */
         do {
            bFound = 0;
            for (i = 0; i < MAX_OUT_OF_ORDER_SIZE; i++) {
               if (aSlots[i].bUsed && aSlots[i].uiSeqNum == uiNextExpected) {
                  if ((int)(aSlots[i].uiSeqNum + aSlots[i].usDataSize) <= MAX_FILE_SIZE)
                     memcpy(pFileBuf + aSlots[i].uiSeqNum,
                            aSlots[i].pData,
                            aSlots[i].usDataSize);
                  uiNextExpected = aSlots[i].uiSeqNum + aSlots[i].usDataSize;
                  if (aSlots[i].bIsFin)
                     bFinReceived = 1;
                  free(aSlots[i].pData);
                  aSlots[i].pData = NULL;
                  aSlots[i].bUsed = 0;
                  bFound = 1;
                  printf("  [drained slot seq=%u]\n", aSlots[i].uiSeqNum);
                  break;
               }
            }
         } while (bFound);

         /* Set total only after all contiguous slots are drained */
         if (bFinReceived)
            uiTotalBytes = uiNextExpected;

      } else if (uiSeqNum > uiNextExpected) {

         /* Out-of-order: store in first free slot */
         for (i = 0; i < MAX_OUT_OF_ORDER_SIZE; i++) {
            if (!aSlots[i].bUsed) {
               aSlots[i].pData = (char *)malloc(usDataSize + 1);
               if (aSlots[i].pData != NULL) {
                  memset(aSlots[i].pData, 0, usDataSize + 1);
                  memcpy(aSlots[i].pData, pPacket, usDataSize);
                  aSlots[i].bUsed = 1;
                  aSlots[i].bIsFin = (ucFlags & TCP_FLAG_FIN) ? 1 : 0;
                  aSlots[i].uiSeqNum = uiSeqNum;
                  aSlots[i].usDataSize = usDataSize;
                  printf("[buffered out-of-order seq=%u]\n", uiSeqNum);
               }
               break;
            }
         }

      }
      /* else: seq < uiNextExpected it's a duplicate, discard */
      free(pPacket);
      pPacket = NULL;

      if (bFinReceived && uiNextExpected == uiTotalBytes) break;
   }

   saveFile(pFileBuf, (int)uiNextExpected);

   for (i = 0; i < MAX_OUT_OF_ORDER_SIZE; i++) {
      if (aSlots[i].bUsed && aSlots[i].pData != NULL)
         free(aSlots[i].pData);
   }
   free(pFileBuf);
}

/* ------------------------------------------------------------------------------------*/
/* Main                                                                                */
/* ------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
   char szServer[64] = {0};
   int  iPort = 0;
   int  iSockFd = 0;
   int  i = 0;

   /* Build the CRC lookup table once before any checksum is computed. */
   F_CRC_InicializaTabla();

   for (i = 1; i < argc - 1; i++) {
      if (strcmp(argv[i], "-server") == 0) {
         strncpy(szServer, argv[i + 1], sizeof(szServer) - 1);
      } else if (strcmp(argv[i], "-port") == 0) {
         iPort = atoi(argv[i + 1]);
      }
   }

   if (szServer[0] == '\0' || iPort == 0) {
      fprintf(stderr, "Usage: %s -server <ip> -port <port>\n", argv[0]);
      return EXIT_FAILURE;
   }

   printf("Connecting to %s:%d\n", szServer, iPort);

   iSockFd = connectToServer(szServer, iPort);
   if (iSockFd < 0) return EXIT_FAILURE;

   receiveFile(iSockFd);

   close(iSockFd);
   return EXIT_SUCCESS;
}
