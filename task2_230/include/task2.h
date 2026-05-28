#ifndef TASK2_H
#define TASK2_H

#include <stdbool.h>

/* Struct to hold metadata about each number read from the input file */
struct TASK2_NUMBERS_METADATA {
    int iIndex;
    int iNumber;         
    int bIsFibonacci;
    int bIsPrimeNumber;
    int bIsSquareNumber;
    int bIsCubeNumber;
    int bIsPerfectNumber;
    int bIsAbundantNumber;
    int bIsDeficientNumber;
    int bIsOddNumber;
};

/* Function prototypes for EWA */
bool isFibonacci(int n);
bool isPrime(int n);
bool isSquareNumber(int n);
bool isCubeNumber(int n);
bool isPerfectNumber(int n);
bool isAbundantNumber(int n);
bool isDeficientNumber(int n);
bool isOdd(int n);

/* Function prototypes for print helpers (task2_print.c) */
void printHeader(void);
void printEntry(struct TASK2_NUMBERS_METADATA *pEntry);
void printFooter(int iCount, const char *pszOutputFile);

#endif /* TASK2_H */

