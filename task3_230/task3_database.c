/* -------------------------------------------------------------------------------------

 Project:     PG3401 Exam Task 3
 Author:      Candidate 230
 Description: Double-linked list operations for the exam gradebook.
              Handles creation, insertion, assessment, file loading,
              grade calculation and cleanup.

/*-------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/

#include "task3.h"

/* Max points per task, used for validation in AssessTask */
const int g_iTaskMax[NUM_TASKS] = {
   TASK1_MAX_POINTS, TASK2_MAX_POINTS, TASK3_MAX_POINTS,
   TASK4_MAX_POINTS, TASK5_MAX_POINTS
};

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/


/* Returns 1 if string contains only digits, 0 otherwise */
static int IsNumericOnly(const char *psz){
   if (psz == NULL || *psz == '\0')
      return 0;

   while (*psz != '\0'){
      if (!isdigit((unsigned char)*psz))
         return 0;
      psz++;
   }
   return 1;
}


/* Find a submission by candidate ID                      
   Returns pointer to SUBMISSION, or NULL if not found */
static SUBMISSION *FindCandidate(GRADEBOOK **ppGradebook,
   const char *pszCandidateId){
   SUBMISSION *pPtr = NULL;

   if (*ppGradebook == NULL || pszCandidateId == NULL)
      return NULL;

   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      if (strncmp(pPtr->szCandidateId, pszCandidateId, MAX_CANDIDATE_ID - 1) == 0)
         return pPtr;
      pPtr = pPtr->pNext;
   }
   return NULL;
}


/* Recalculate iScore as sum of all graded tasks */
static void RecalculateScore(SUBMISSION *pSub){
   int i = 0;
   int iSum = 0;

   for (i = 0; i < NUM_TASKS; i++){
      if (pSub->iPoints[i] != NOT_GRADED)
      iSum += pSub->iPoints[i];
   }
   pSub->iScore = iSum;
}

/* Returns the letter grade for a given total score (A-F scale) */
char GetGrade(int iScore){
   if (iScore >= GRADE_A_MIN) return 'A';
   if (iScore >= GRADE_B_MIN) return 'B';
   if (iScore >= GRADE_C_MIN) return 'C';
   if (iScore >= GRADE_D_MIN) return 'D';
   if (iScore >= GRADE_E_MIN) return 'E';
   return 'F';
}


/* Allocate and initialise an empty gradebook                                 
   Returns 0 on success, -1 on failure */
int CreateGradebook(GRADEBOOK **ppGradebook){
   *ppGradebook = malloc(sizeof(GRADEBOOK));
   if (*ppGradebook == NULL){
      fprintf(stderr, "ERROR: Could not allocate gradebook\n");
      return -1;
   }
   memset(*ppGradebook, 0, sizeof(GRADEBOOK));
   return 0;
}


/* Allocate and wire a new SUBMISSION node into the list */
static int InsertSubmission(GRADEBOOK **ppGradebook, const char *pszCandidateId){
   SUBMISSION *pSub = NULL;
   int i = 0;

   pSub = malloc(sizeof(SUBMISSION));
   if (pSub == NULL){
      fprintf(stderr, "ERROR: Could not allocate submission\n");
      return -1;
   }
   memset(pSub, 0, sizeof(SUBMISSION));

   strncpy(pSub->szCandidateId, pszCandidateId, MAX_CANDIDATE_ID - 1);
   pSub->szCandidateId[MAX_CANDIDATE_ID - 1] = '\0';
   pSub->iScore = 0;

   for (i = 0; i < NUM_TASKS; i++){
      pSub->iPoints[i] = NOT_GRADED;
      pSub->szJustification[i][0] = '\0';
   }

   /* Append to tail */
   pSub->pPrev = (*ppGradebook)->pTail;
   pSub->pNext = NULL;

   if ((*ppGradebook)->pTail != NULL)
      (*ppGradebook)->pTail->pNext = pSub;

   (*ppGradebook)->pTail = pSub;

   if ((*ppGradebook)->pHead == NULL)
      (*ppGradebook)->pHead = pSub;
   /* Brackets are there because of operator precedence. Without them, *ppGradebook->pTail would
      be parsed as *(ppGradebook->pTail). And you can't -> on a pointer, only a struct. */
   (*ppGradebook)->iSize++;
   return 0;
}

/* Add a candidate with a specific ID (must be digits only, no duplicates)    
   Returns 0 on success, -1 on failure */
int AddCandidate(GRADEBOOK **ppGradebook, const char *pszCandidateId){
   if (*ppGradebook == NULL || pszCandidateId == NULL || *pszCandidateId == '\0')
      return -1;

   if (!IsNumericOnly(pszCandidateId) || strlen(pszCandidateId) != 4){
      printf("Invalid ID '%s': candidate ID must be exactly 4 digits.\n",
      pszCandidateId);
      return -1;
   }

   if (FindCandidate(ppGradebook, pszCandidateId) != NULL){
      printf("Candidate '%s' already exists.\n", pszCandidateId);
      return -1;
   }

   if (InsertSubmission(ppGradebook, pszCandidateId) != 0)
      return -1;

   printf("Added candidate: %s\n", pszCandidateId);
      return 0;
}

/* Load candidate IDs from a text file (one ID per line, digits only)         
   Returns number of candidates added, -1 on file error */
int LoadCandidatesFromFile(GRADEBOOK **ppGradebook, const char *pszFilename){
   FILE *pFile = NULL;
   char szBuf[BUFFER_SIZE];
   char *pNewline = NULL;
   int iAdded = 0;

   if (*ppGradebook == NULL || pszFilename == NULL)
      return -1;

   pFile = fopen(pszFilename, "r");
   if (pFile == NULL){
      fprintf(stderr, "ERROR: Could not open file '%s'\n", pszFilename);
      return -1;
   }

   while (fgets(szBuf, sizeof(szBuf), pFile) != NULL){
      pNewline = strchr(szBuf, '\n');
      if (pNewline != NULL)
         *pNewline = '\0';

      if (szBuf[0] == '\0')
         continue;

      if (AddCandidate(ppGradebook, szBuf) == 0)
         iAdded++;
   }
   fclose(pFile);
   printf("Loaded %d candidate(s) from '%s'.\n", iAdded, pszFilename);

   return iAdded;
}

/*  Set points and justification for one task on one candidate                 
    iTask is 1-based (1..NUM_TASKS)                                            
    Returns 0 on success, -1 on failure */
int AssessTask(GRADEBOOK **ppGradebook, const char *pszCandidateId,
               int iTask, int iPoints, const char *pszJustification){
   SUBMISSION *pSub = NULL;
   int iIdx = iTask - 1;

   if (*ppGradebook == NULL || pszCandidateId == NULL || pszJustification == NULL)
      return -1;

   if (iTask < 1 || iTask > NUM_TASKS){
      printf("Invalid task number %d (must be 1-%d).\n", iTask, NUM_TASKS);
      return -1;
   }

   if (iPoints < 0 || iPoints > g_iTaskMax[iIdx]){
      printf("Invalid points %d for task %d (max: %d).\n",
      iPoints, iTask, g_iTaskMax[iIdx]);
      return -1;
   }

   pSub = FindCandidate(ppGradebook, pszCandidateId);
   if (pSub == NULL){
      printf("Candidate '%s' not found.\n", pszCandidateId);
      return -1;
   }

   pSub->iPoints[iIdx] = iPoints;
   strncpy(pSub->szJustification[iIdx], pszJustification, MAX_JUSTIFICATION - 1);
   pSub->szJustification[iIdx][MAX_JUSTIFICATION - 1] = '\0';

   RecalculateScore(pSub);

   printf("Task %d assessed for %s: %d/%d pts\n",
   iTask, pszCandidateId, iPoints, g_iTaskMax[iIdx]);
   return 0;
}


/* Assign random points to every task for every candidate in the gradebook.  
   Seeds the RNG once per call, then sets a random score within each task's  
   allowed range and a generic justification. Recalculates totals.           
   Returns number of candidates graded, -1 on error */
int RandomGradeAll(GRADEBOOK **ppGradebook){
   SUBMISSION *pPtr = NULL;
   int i = 0;
   int iCount = 0;

   if (*ppGradebook == NULL)
    return -1;

   if ((*ppGradebook)->iSize == 0){
      printf("Gradebook is empty - no candidates to grade.\n");
      return 0;
   }

   srand((unsigned int)time(NULL));

   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      for (i = 0; i < NUM_TASKS; i++){
         pPtr->iPoints[i] = rand() % (g_iTaskMax[i] + 1);
         strncpy(pPtr->szJustification[i], "Randomly graded",
         MAX_JUSTIFICATION - 1);
         pPtr->szJustification[i][MAX_JUSTIFICATION - 1] = '\0';
      }
      RecalculateScore(pPtr);
      iCount++;
      pPtr = pPtr->pNext;
   }
   printf("Randomly graded %d candidate(s).\n", iCount);
   return iCount;
}

/*  Free all submissions and the gradebook itself */
void DestroyGradebook(GRADEBOOK **ppGradebook){
   SUBMISSION *pPtr = NULL;
   SUBMISSION *pTemp = NULL;

   if (*ppGradebook == NULL)
      return;

   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      pTemp = pPtr;
      pPtr  = pPtr->pNext;
      free(pTemp);
      pTemp = NULL;
   }

   free(*ppGradebook);
   *ppGradebook = NULL;
   printf("Gradebook destroyed. All memory freed.\n");
}

