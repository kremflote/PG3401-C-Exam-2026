/* --------------------------------------------------------------------------------------

 Project:     PG3401 Exam Task 2
 Author:      Candidate 230
 Description: Reads integers from pgexam26_test.txt, evaluates each
              number using the EWA-provided functions, and writes the
              results as binary structs to pgexam26_output.bin

/*-------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "task2.h"

/* ------------------------------------------------------------------------------------*/
/* Defines                                                                             */
/* ------------------------------------------------------------------------------------*/

#define INPUT_FILE "pgexam26_test.txt"
#define OUTPUT_FILE "pgexam26_output.bin"
#define BUFFER_SIZE 32


/* ------------------------------------------------------------------------------------*/
/* Main                                                                                */
/* ------------------------------------------------------------------------------------*/

int main(void) {
   FILE *pInputFile = NULL;
   FILE *pOutputFile = NULL;
   struct TASK2_NUMBERS_METADATA entry;
   char szBuffer[BUFFER_SIZE];
   char *pEnd = NULL;
   long lNumber = 0;
   int iIndex = 0;
   int iCount = 0;

   /* Helpermethod for printing a fancy intro in the terminal */
   printHeader();

   /* Open input file for reading */
   pInputFile = fopen(INPUT_FILE, "r");
   if (pInputFile == NULL) {
      fprintf(stderr, "Error: could not open input file '%s'\n", INPUT_FILE);
      return EXIT_FAILURE;
   }

   /* Open output file for binary writing */
   pOutputFile = fopen(OUTPUT_FILE, "wb");
   if (pOutputFile == NULL) {
      fprintf(stderr, "Error: could not open output file '%s'\n", OUTPUT_FILE);
      fclose(pInputFile);
      return EXIT_FAILURE;
   }

   /* Read one line at a time, parse as integer with strtol */
   while (fgets(szBuffer, sizeof(szBuffer), pInputFile) != NULL) {
      lNumber = strtol(szBuffer, &pEnd, 10);

      /* if strtol made no progress, it means there was nothing to read,
      and pEnd will point at the same adress as szBuffer*/
      if (pEnd == szBuffer) {
         continue;
      }
      /* Since the course syllabus encourages the c89 standard where possible,
      We cast the boolean into an int when reading from the input file. */
      entry.iIndex = iIndex;
      entry.iNumber = (int)lNumber;
      entry.bIsFibonacci = (int)isFibonacci(entry.iNumber);
      entry.bIsPrimeNumber = (int)isPrime(entry.iNumber);
      entry.bIsSquareNumber = (int)isSquareNumber(entry.iNumber);
      entry.bIsCubeNumber = (int)isCubeNumber(entry.iNumber);
      entry.bIsPerfectNumber = (int)isPerfectNumber(entry.iNumber);
      entry.bIsAbundantNumber = (int)isAbundantNumber(entry.iNumber);
      entry.bIsDeficientNumber = (int)isDeficientNumber(entry.iNumber);
      entry.bIsOddNumber = (int)isOdd(entry.iNumber);

      printEntry(&entry);

      /* Write struct as binary to output file */
      if (fwrite(&entry, sizeof(struct TASK2_NUMBERS_METADATA), 1, pOutputFile) != 1) {
         fprintf(stderr, "Error: failed to write entry %d to output file\n", iIndex);
         fclose(pInputFile);
         fclose(pOutputFile);
         return EXIT_FAILURE;
      }
      iIndex++;
      iCount++;
   }
   fclose(pInputFile);
   fclose(pOutputFile);

   printFooter(iCount, OUTPUT_FILE);

   return EXIT_SUCCESS;
}
