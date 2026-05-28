#ifndef TASK3_H
#define TASK3_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------*/
/* Exam structure                                                             */
/* ---------------------------------------------------------------------------*/
#define NUM_TASKS 5
#define TASK1_MAX_POINTS 15
#define TASK2_MAX_POINTS 15
#define TASK3_MAX_POINTS 20
#define TASK4_MAX_POINTS 25
#define TASK5_MAX_POINTS 25
#define TOTAL_MAX_POINTS 100

/* ---------------------------------------------------------------------------*/
/* Buffer sizes                                                               */
/* ---------------------------------------------------------------------------*/
#define MAX_CANDIDATE_ID 16
#define MAX_JUSTIFICATION 128
#define BUFFER_SIZE 64

/* iPoints value meaning a task has not been graded yet */
#define NOT_GRADED -1

/* ---------------------------------------------------------------------------*/
/* Norwegian A-F grading thresholds (percentage of TOTAL_MAX_POINTS)          */
/* ---------------------------------------------------------------------------*/
#define GRADE_A_MIN 90
#define GRADE_B_MIN 77
#define GRADE_C_MIN 61
#define GRADE_D_MIN 41
#define GRADE_E_MIN 26

/* ---------------------------------------------------------------------------*/
/* Data structures                                                            */
/* ---------------------------------------------------------------------------*/

/* Node in the double-linked list */
typedef struct _SUBMISSION {
    char szCandidateId[MAX_CANDIDATE_ID];
    int iPoints[NUM_TASKS];               /* #define NOT_GRADED = unassessed  */
    char szJustification[NUM_TASKS][MAX_JUSTIFICATION];
    int iScore;                           /* Running sum of graded tasks      */
    struct _SUBMISSION *pNext;
    struct _SUBMISSION *pPrev;
} SUBMISSION;

/* Double-linked list container */
typedef struct _GRADEBOOK {
    SUBMISSION *pHead;
    SUBMISSION *pTail;
    int iSize;
} GRADEBOOK;

/* ---------------------------------------------------------------------------*/
/* Function prototypes - database operations (task3_database.c)               */
/* ---------------------------------------------------------------------------*/
int CreateGradebook(GRADEBOOK **ppGradebook);
int AddCandidate(GRADEBOOK **ppGradebook, const char *pszCandidateId);
int AddCandidateAuto(GRADEBOOK **ppGradebook);
int LoadCandidatesFromFile(GRADEBOOK **ppGradebook, const char *pszFilename);
int AssessTask(GRADEBOOK **ppGradebook, const char *pszCandidateId,
               int iTask, int iPoints, const char *pszJustification);
int RandomGradeAll(GRADEBOOK **ppGradebook);
void DestroyGradebook(GRADEBOOK **ppGradebook);
/* Defined once in task3_database.c. Extern so all files share one copy  
   Global variables are risky, but this is an unchanging const representing 
   max points per task.                                                       */
extern const int g_iTaskMax[NUM_TASKS];
char GetGrade(int iScore);

/* ---------------------------------------------------------------------------*/
/* Function prototypes - print operations (task3_print.c)                     */
/* ---------------------------------------------------------------------------*/
void PrintAllCandidates(GRADEBOOK **ppGradebook);
void PrintByGrade(GRADEBOOK **ppGradebook, char cGrade);
void PrintUngraded(GRADEBOOK **ppGradebook);
void PrintCandidateDetail(GRADEBOOK **ppGradebook, const char *pszCandidateId);
void printMenu(void);
void printHeader(void);

#endif /* TASK3_H */

