/* --------------------------------------------------------------------------------------

Project:     PG3401 Exam Task 2
Author:      Candidate 230
Description: Print helper functions for displaying number classification
             results in a framed terminal layout

/*-------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/

#include <stdio.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "task2.h"

/* ------------------------------------------------------------------------------------*/
/* Defines                                                                             */
/* ------------------------------------------------------------------------------------*/

#define SEP_FULL "+------------------------------------------------------------+"
#define SEP_HALF "+------------------------------+-----------------------------+"
#define SEP_TOP  "+============================================================+"

/* ------------------------------------------------------------------------------------*/
/* Functions                                                                           */
/* ------------------------------------------------------------------------------------*/

/* Prints a framed header describing what the program does */
void printHeader(void) {
   printf("%s\n", SEP_TOP);
   printf("|        PG3401 Exam 2026 - Task 2: Number Classifier        |\n");
   printf("|  Reads integers from pgexam26_test.txt, calls 8 functions  |\n");
   printf("|  per number and writes binary structs to output file.      |\n");
   printf("%s\n\n", SEP_TOP);
}

/* Prints a framed summary of all classification results for one number */
void printEntry(struct TASK2_NUMBERS_METADATA *pEntry) {
   printf("%s\n", SEP_FULL);
   printf("|  [%02d] Number: %-10d                                   |\n",
          pEntry->iIndex, pEntry->iNumber);
   printf("%s\n", SEP_HALF);
   printf("|  %-11s: %-3s           |  %-11s: %-4s           |\n",
          "Fibonacci",  pEntry->bIsFibonacci      ? "YES" : "NO",
          "Prime",      pEntry->bIsPrimeNumber    ? "YES" : "NO");
   printf("|  %-11s: %-3s           |  %-11s: %-4s           |\n",
          "Square",     pEntry->bIsSquareNumber   ? "YES" : "NO",
          "Cube",       pEntry->bIsCubeNumber     ? "YES" : "NO");
   printf("|  %-11s: %-3s           |  %-11s: %-4s           |\n",
          "Perfect",    pEntry->bIsPerfectNumber  ? "YES" : "NO",
          "Abundant",   pEntry->bIsAbundantNumber ? "YES" : "NO");
   printf("|  %-11s: %-3s           |  %-11s: %-4s           |\n",
          "Deficient",  pEntry->bIsDeficientNumber ? "YES" : "NO",
          "Odd",        pEntry->bIsOddNumber      ? "YES" : "NO");
   printf("%s\n\n", SEP_HALF);
}

/* Prints a framed summary footer */
void printFooter(int iCount, const char *pszOutputFile) {
   printf("%s\n", SEP_TOP);
   printf("|  Done! Processed %-2d numbers.                               |\n", iCount);
   printf("|  Binary output -> %-41s|\n", pszOutputFile);
   printf("%s\n", SEP_TOP);
}

