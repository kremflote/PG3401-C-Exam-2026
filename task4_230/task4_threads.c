/* This file has been created by EWA, and is part of task 4 on the exam for PG3401 2026*/
/* --------------------------------------------------------------------------------------

 Project:       PG3401 Exam Task 4
 Modified by:   Candidate 230
 Description:   Uses two parallel worker threads to search a file of large integers
                for prime numbers. For each prime found, the threads attempt to decrypt
                task4_code.bin using XTEA with that prime as the key. If the decrypted
                output starts with "BENGT" the key has been found.
                The plaintext is saved to task4_plain.txt and an IP checksum is written
                to task4_plain.hash.

                Thread coordination uses one semaphore and two mutexes:
                  mutexFile: protects the shared file handle
                  mutexState: protects the shared prime list and iKeyFound flag
				  semKeyFound: signals to main thread that key is found

                Both threads push discovered primes onto a single shared linked list
                and pop from it to attempt decryption. mutexState ensures only one
                thread modifies the list or iKeyFound at a time. Any nodes remaining
                after both threads exit are freed by main.

 Comments:      I initially solved this only using mutex, but wanted to use
                semaphores. In hindsight it feels a bit forced, I could easily just check
                iKeyFound after the joins. However abstraction-wise it might make sense
                to "signal" operation success from thread to main thread using semaphore.
                Leaving it as to demonstrate knowledge of semaphore, but I prefer the
                mutex-only solution.

 Usage:         ./task4 <primes_file.txt>

---------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "task4_prime.h"

/* ------------------------------------------------------------------------------------*/
/* Forward declarations                                                                */
/* ------------------------------------------------------------------------------------*/

/* EWA-provided source files have no headers, declared here instead */
void tean(unsigned int *const v, unsigned int *const w,
          const unsigned int *const k, int N);
unsigned int tcp_checksum(const unsigned char *data, size_t length);

/* ------------------------------------------------------------------------------------*/
/* Defines                                                                             */
/* ------------------------------------------------------------------------------------*/

#define CHUNK_SIZE 10       /* The size specified in the original file                 */
#define ENC_FILENAME "task4_code.bin"
#define OUT_FILENAME "task4_plain.txt"
#define HASH_FILENAME "task4_plain.hash"
#define XTEA_BLOCK_SIZE 8   /* XTEA operates on 8-byte blocks (two unsigned ints)      */
#define XTEA_ROUNDS 32      /* Standard XTEA: 32 loop iterations = 64 mixing ops       */
#define BUFFER 32           /* For fgets.                                              */

/* ------------------------------------------------------------------------------------*/
/* Structs                                                                             */
/* ------------------------------------------------------------------------------------*/

/* Single-linked list node. Primes found by either thread are saved in this struct     */
struct PGPRIMENUMBER {
   unsigned int uiPrime;
   struct PGPRIMENUMBER* next;
};

/* Introduce ThreadArgs. Since the pthread interface mandates
   void* (*start_routine)(void*) shape, we need to bundle necessary parameters
   into a single pointer which we can cast back on the other side                      */
struct ThreadArgs {
   FILE* fInputFile;              /* Shared handle to the primes text file             */
   unsigned char* pEncData;       /* Encrypted file loaded into memory                 */
   size_t szEncSize;              /* Size of encrypted data in bytes                   */
   int iKeyFound;                 /* Set to 1 by whichever thread cracks the key       */
   struct PGPRIMENUMBER* pHead;   /* Head of the shared prime list                     */
   pthread_mutex_t mutexFile;     /* Protects fInputFile                               */
   pthread_mutex_t mutexState;     /* Protects pHead and iKeyFound                     */
   sem_t semKeyFound;             /* thread -> main: key found                         */
};

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/

/* Reads a binary file into a malloc'd buffer. Caller must free.
   Returns NULL on failure, sets *pszSize to the number of bytes read.                */
static unsigned char* readFile(const char *pszFilename, size_t *pszSize) {
   FILE* pFile = NULL;
   unsigned char* pData = NULL;
   long lSize = 0;

   pFile = fopen(pszFilename, "rb");
   if (pFile == NULL) return NULL;

   fseek(pFile, 0, SEEK_END);
   lSize = ftell(pFile);
   fseek(pFile, 0, SEEK_SET);

   if (lSize <= 0) { fclose(pFile); return NULL; }

   pData = malloc((size_t)lSize);
   if (pData == NULL) { fclose(pFile); return NULL; }

   if (fread(pData, 1, (size_t)lSize, pFile) != (size_t)lSize) {
      free(pData);
      fclose(pFile);
      return NULL;
   }

   fclose(pFile);
   *pszSize = (size_t)lSize;
   return pData;
}

/* Attempts to decrypt pData using uiPrime as the XTEA key.
   Result is written into pOut (caller must provide buffer of at least szSize bytes).
   Returns 1 if first 5 bytes of result are "BENGT", 0 otherwise.                   
   Could be more efficient. This function decrypts whole file before checking for bengt
   a more efficient method would be to callate first 5, and if true calcualte the rest */
static int checkBengt(unsigned int uiPrime, const unsigned char *pData, size_t szSize, unsigned char *pOut) {
   unsigned int key[4] = {uiPrime, uiPrime, uiPrime, uiPrime};
   unsigned int input[2] = {0, 0};
   unsigned int output[2] = {0, 0};
   size_t i = 0;

   for (i = 0; i + XTEA_BLOCK_SIZE <= szSize; i += XTEA_BLOCK_SIZE) {
      memcpy(input, pData + i, XTEA_BLOCK_SIZE);
      tean(input, output, key, -XTEA_ROUNDS); /* Negative N = decrypt */
      memcpy(pOut + i, output, XTEA_BLOCK_SIZE);
   }

   return (szSize >= 5 &&
      pOut[0] == 'B' &&
      pOut[1] == 'E' &&
      pOut[2] == 'N' &&
      pOut[3] == 'G' &&
      pOut[4] == 'T');
}

/* Writes the decrypted plaintext to task4_plain.txt and the IP checksum
   of that plaintext to task4_plain.hash.                                             */
static void writeResults(const unsigned char *pData, size_t szSize) {
   FILE* pFile = NULL;
   unsigned int uiChecksum = 0;

   pFile = fopen(OUT_FILENAME, "wb");
   if (pFile != NULL) {
      fwrite(pData, 1, szSize, pFile);
      fclose(pFile);
      printf("Decrypted file saved to %s\n", OUT_FILENAME);
   }

   uiChecksum = tcp_checksum(pData, szSize);
   pFile = fopen(HASH_FILENAME, "w");
   if (pFile != NULL) {
      fprintf(pFile, "%u\n", uiChecksum);
      fclose(pFile);
      printf("Checksum %u written to %s\n", uiChecksum, HASH_FILENAME);
   }
}

/* Allocates a new node and pushes uiPrime onto the front of the shared list         */
static void pushPrime(struct ThreadArgs *args, unsigned int uiPrime) {
   struct PGPRIMENUMBER* pNode = malloc(sizeof(struct PGPRIMENUMBER));
   if (pNode == NULL) return;
   pNode->uiPrime = uiPrime;

   pthread_mutex_lock(&args->mutexState);
   pNode->next = args->pHead;
   args->pHead = pNode;
   pthread_mutex_unlock(&args->mutexState);
}

/* Removes and returns the head node from the shared list, or NULL if empty.
   Caller is responsible for freeing the returned node.                               */
static struct PGPRIMENUMBER* popPrime(struct ThreadArgs *args) {
   struct PGPRIMENUMBER* pNode = NULL;
   pthread_mutex_lock(&args->mutexState);
   if (args->pHead != NULL) {
      pNode = args->pHead;
      args->pHead = pNode->next;
      pNode->next = NULL;
   }
   pthread_mutex_unlock(&args->mutexState);

   return pNode;
}

/* ------------------------------------------------------------------------------------*/
/* Thread function                                                                     */
/* ------------------------------------------------------------------------------------*/

/* Parameter 3 in pthread_create takes this function pointer.
   Both threads run this same function concurrently.
   Each iteration: read a chunk from the file, push discovered primes onto the
   shared list, then pop and attempt decryption until the list is empty.               */
void* threadFunction(void* arg){
   struct ThreadArgs* args = (struct ThreadArgs*)arg;
   unsigned int auiNumbers[CHUNK_SIZE] = {0};
   int iNumbersRead = 0;
   int iIndex = 0;
   char szLine[32];
   int bShouldWrite = 0;          /* Double-check flag: only the thread that sets iKeyFound=1 writes results */
   unsigned int uiFoundKey = 0;   /* Holds the winning prime after pNode is freed, for printing             */
   struct PGPRIMENUMBER* pNode = NULL;

   /* Allocate output buffer once per thread. */
   unsigned char* pOut = malloc(args->szEncSize);
   if (pOut == NULL) return NULL;
   
   memset(pOut, 0, args->szEncSize); /* Defensive memset */

   /* iKeyFound is read without a lock at three poll sites below. This is technically
      a data race, but is intentional. locking every time we check it is expensive,
	  especially for simply avoiding wasting a marginal amount of compute.
	  The practical consequence is that the other thread may print "Found prime X, attempting decrypt..."
	  one extra time after the key has already been found and written                  */
   while (1) {
      if (args->iKeyFound) break;

      iNumbersRead = 0;

      /* Lock before reading so only one thread advances the file position at a time.
         Without this, both threads could read the same numbers or skip numbers
         depending on how the OS schedules them.                                       */
      pthread_mutex_lock(&args->mutexFile);
      while (iNumbersRead < CHUNK_SIZE) {
         if (fgets(szLine, sizeof(szLine), args->fInputFile) == NULL) break;
         auiNumbers[iNumbersRead] = (unsigned int)strtoul(szLine, NULL, 10);
         iNumbersRead++;
      }
      pthread_mutex_unlock(&args->mutexFile);

      if (iNumbersRead == 0) break; /* EOF */

      /* Test primality and push primes onto the shared list.
         isPrime is pure computation. no lock needed.                                */
      for (iIndex = 0; iIndex < iNumbersRead; iIndex++) {
         if (args->iKeyFound) break;
         if (isPrime(auiNumbers[iIndex])) {
            pushPrime(args, auiNumbers[iIndex]);
         }
      }

      /* Pop and attempt decryption for each prime currently in the list.
         The other thread may also be popping concurrently — popPrime handles this.   */
      while ((pNode = popPrime(args)) != NULL) {
         if (args->iKeyFound) { free(pNode); break; }

         printf("Found prime %u, attempting decrypt...\n", pNode->uiPrime);
         fflush(stdout);

         if (checkBengt(pNode->uiPrime, args->pEncData, args->szEncSize, pOut)) {
            bShouldWrite = 0;
            pthread_mutex_lock(&args->mutexState);
            if (!args->iKeyFound) {
               args->iKeyFound = 1;
               bShouldWrite    = 1;
            }
            pthread_mutex_unlock(&args->mutexState);

            uiFoundKey = pNode->uiPrime; /* Save before free, pNode must not be read after this */
            free(pNode);

            if (bShouldWrite) {
               printf("Key found: %u\n", uiFoundKey);
               writeResults(pOut, args->szEncSize);
               sem_post(&args->semKeyFound); /* Signal main: key found */
            }

            free(pOut);
            return NULL;
         }

         free(pNode);
      }
   }

   free(pOut);
   return NULL;
}

/* ------------------------------------------------------------------------------------*/
/* Main: loads encrypted file, spawns two worker threads, waits for both to finish.    */
/* ------------------------------------------------------------------------------------*/
int main(int argc, char* argv[]){
   pthread_t thread1;
   pthread_t thread2;
   int rc = 0;
   size_t szEncSize = 0;
   

   if (argc < 2) {
      fprintf(stderr, "Usage: %s <primes_file>\n", argv[0]);
      return EXIT_FAILURE;
   }

   FILE* fInputFile = fopen(argv[1], "r"); /* Filename passed as command line argument */
   if (fInputFile == NULL) {
      fprintf(stderr, "Error: could not open file '%s'\n", argv[1]);
      return EXIT_FAILURE;
   }

   /* Load encrypted file into memory once. Both threads read from this buffer        */
   unsigned char* pEncData = readFile(ENC_FILENAME, &szEncSize);
   if (pEncData == NULL) {
      fprintf(stderr, "Error: could not read '%s'\n", ENC_FILENAME);
      fclose(fInputFile);
      return EXIT_FAILURE;
   }

   /* Prepare struct for passing through pthread_create as void*                      */
   struct ThreadArgs args;
   args.fInputFile = fInputFile;
   args.pEncData = pEncData;
   args.szEncSize = szEncSize;
   args.iKeyFound = 0;
   args.pHead = NULL;

   pthread_mutex_init(&args.mutexFile, NULL);
   pthread_mutex_init(&args.mutexState, NULL);
   sem_init(&args.semKeyFound, 0, 0); /* pshared=0 (threads only), value=0 (not yet signalled) */

   /* Creates the first thread, and stores the outcome in rc                          */
   rc = pthread_create(&thread1, NULL, threadFunction, &args);
   if (rc != 0) {
      pthread_mutex_destroy(&args.mutexFile);
      pthread_mutex_destroy(&args.mutexState);
      sem_destroy(&args.semKeyFound);
      fclose(fInputFile);
      free(pEncData);
      return EXIT_FAILURE;
   }

   /* Create the second thread                                                        */
   rc = pthread_create(&thread2, NULL, threadFunction, &args);
   if (rc != 0) {
      pthread_join(thread1, NULL); /* thread1 already running, wait before cleaning up */
      pthread_mutex_destroy(&args.mutexFile);
      pthread_mutex_destroy(&args.mutexState);
      sem_destroy(&args.semKeyFound);
      fclose(fInputFile);
      free(pEncData);
      return EXIT_FAILURE;
   }

   /* Block main until both worker threads have finished */
   pthread_join(thread1, NULL);
   pthread_join(thread2, NULL);

   /* Both threads have returned, no concurrency, safe to free list without a lock.
      Nodes pushed but never popped (if the key was found mid-chunk) are freed here. */
   struct PGPRIMENUMBER *pNode = args.pHead;
   struct PGPRIMENUMBER *pNext = NULL;
   while (pNode != NULL) {
      pNext = pNode->next;
      free(pNode);
      pNode = pNext;
   }
   

   /* sem_trywait returns 0 if the semaphore was posted (key found), non-zero otherwise */
   if (sem_trywait(&args.semKeyFound) != 0) {
      fprintf(stderr, "Key not found: no prime in the file decrypted the message.\n");
   }

   pthread_mutex_destroy(&args.mutexFile);
   pthread_mutex_destroy(&args.mutexState);
   sem_destroy(&args.semKeyFound);
   fclose(fInputFile);
   free(pEncData);

   return EXIT_SUCCESS;
}

