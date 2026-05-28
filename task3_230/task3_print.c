/* -------------------------------------------------------------------------------------

 Project: PG3401 Exam Task 3
 Author: Candidate 230
 Description: Display functions for the exam gradebook. Handles
              printing all candidates, filtering by grade, listing
              ungraded candidates, and showing full candidate detail.

/*-------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/
#include <stdio.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/
#include "task3.h"

/* ------------------------------------------------------------------------------------*/
/* Defines                                                                             */
/* ------------------------------------------------------------------------------------*/
#define SEP_WIDE  "+--------+----+----+----+----+----+-------+-------+"
#define SEP_MID   "+------------------------------------------+-------+"

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/

/* Returns 1 if all tasks have been assessed, 0 otherwise    */
static int IsFullyGraded(SUBMISSION *pSub){
   int i = 0;
   for (i = 0; i < NUM_TASKS; i++){
      if (pSub->iPoints[i] == NOT_GRADED)
         return 0;
   }
   return 1;
}
/* ------------------------------------------------------------------------------------*/
/* Functions                                                                           */
/* ------------------------------------------------------------------------------------*/

/* Print all candidates with their score and grade  */
void PrintAllCandidates(GRADEBOOK **ppGradebook){
   SUBMISSION *pPtr = NULL;
   int iCount = 0;
   int i = 0;

   if (*ppGradebook == NULL || (*ppGradebook)->iSize == 0){
      printf("Gradebook is empty.\n");
      return;
   }

   printf("\n%s\n", SEP_WIDE);
   printf("| %-6s | T1 | T2 | T3 | T4 | T5 | Score | Grade |\n",
          "CandID");
   printf("%s\n", SEP_WIDE);

   pPtr = (*ppGradebook)->pHead;

   while (pPtr != NULL){
      printf("| %-6s |", pPtr->szCandidateId);
      for (i = 0; i < NUM_TASKS; i++){
         if (pPtr->iPoints[i] == NOT_GRADED){
            printf(" -- |");
         }
         else {
            printf(" %2d |", pPtr->iPoints[i]);
         }
      }
      if (IsFullyGraded(pPtr)){
         printf(" %5d | %5c |\n", pPtr->iScore, GetGrade(pPtr->iScore));
      }
      else {
         printf(" %5d | %5s |\n", pPtr->iScore, "N/A");
      }
      iCount++;
      pPtr = pPtr->pNext;
   }
   printf("%s\n", SEP_WIDE);
   printf("Total candidates: %d\n\n", iCount);
}

/* Print all candidates who have achieved a specific grade */
void PrintByGrade(GRADEBOOK **ppGradebook, char cGrade){
   SUBMISSION *pPtr = NULL;
   int iCount = 0;

   if (*ppGradebook == NULL || (*ppGradebook)->iSize == 0){
      printf("Gradebook is empty.\n");
      return;
   }

   printf("\n  Candidates with grade %c:\n", cGrade);
   printf("%s\n", SEP_MID);
   printf("|  %-40s| Grade |\n", "Candidate ID");
   printf("%s\n", SEP_MID);

   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      if (IsFullyGraded(pPtr) && GetGrade(pPtr->iScore) == cGrade){
         printf("|  %-40s|   %c   |\n",
         pPtr->szCandidateId, cGrade);
         iCount++;
      }
      pPtr = pPtr->pNext;
   }

   printf("%s\n", SEP_MID);
   printf("Found %d candidate(s) with grade %c.\n\n", iCount, cGrade);
}

/* Print all candidates that have not been fully graded yet */
void PrintUngraded(GRADEBOOK **ppGradebook){
   SUBMISSION *pPtr = NULL;
   int iCount = 0;
   int i = 0;

   if (*ppGradebook == NULL || (*ppGradebook)->iSize == 0){
      printf("Gradebook is empty.\n");
      return;
   }

   printf("\nUngraded candidates (missing one or more tasks):\n");
   printf("+--------+----+----+----+----+----+\n");
   printf("| CandID | T1 | T2 | T3 | T4 | T5 |\n");
   printf("+--------+----+----+----+----+----+\n");

   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      if (!IsFullyGraded(pPtr)){
         printf("| %-6s |", pPtr->szCandidateId);
         for (i = 0; i < NUM_TASKS; i++){
            if (pPtr->iPoints[i] == NOT_GRADED)
               printf(" -- |");
            else
               printf(" %2d |", pPtr->iPoints[i]);
         }
         printf("\n");
         iCount++;
      }
      pPtr = pPtr->pNext;
   }
   printf("+--------+----+----+----+----+----+\n");
   printf("Ungraded candidates: %d\n\n", iCount);
}

/* Print full detail for one candidate: score, grade, and per-task breakdown */
void PrintCandidateDetail(GRADEBOOK **ppGradebook, const char *pszCandidateId){
   SUBMISSION *pPtr = NULL;
   int i = 0;

   if (*ppGradebook == NULL || pszCandidateId == NULL)
      return;

   /* Find the candidate */
   pPtr = (*ppGradebook)->pHead;
   while (pPtr != NULL){
      if (strncmp(pPtr->szCandidateId, pszCandidateId, MAX_CANDIDATE_ID - 1) == 0)
         break;
      pPtr = pPtr->pNext;
   }

   if (pPtr == NULL){
      printf("Candidate '%s' not found.\n", pszCandidateId);
      return;
   }

   printf("\n+================================================+\n");
   printf("|  Candidate: %-30s     |\n", pPtr->szCandidateId);

   if (IsFullyGraded(pPtr))
      printf("|  Score: %-3d / %-3d    Grade: %c                  |\n",
             pPtr->iScore, TOTAL_MAX_POINTS, GetGrade(pPtr->iScore));
   else
      printf("|  Score: %-3d / %-3d    Grade: N/A                |\n",
             pPtr->iScore, TOTAL_MAX_POINTS);

   printf("+======+========+================================+\n");
   printf("| Task | Points | Justification                  |\n");
   printf("+======+========+================================+\n");

   for (i = 0; i < NUM_TASKS; i++){
      if (pPtr->iPoints[i] == NOT_GRADED){
         printf("|  T%-2d | %2s/%-2d  | %-30s |\n",
         i + 1, "--", g_iTaskMax[i], "(not assessed)");
      }
      else {
         printf("|  T%-2d | %2d/%-2d  | %-30s |\n",
         i + 1, pPtr->iPoints[i], g_iTaskMax[i],
         pPtr->szJustification[i][0] != '\0'
         ? pPtr->szJustification[i] : "(no justification)");
      }
   }
   printf("+======+========+================================+\n\n");
}

/* Prints a framed header describing what the program does */
void printHeader(void){
   printf("\n+============================================================+\n");
   printf("|       PG3401 Exam 2026 - Task 3: Grading System            |\n");
   printf("+============================================================+\n");
   printf("|                       The Gradebook.                       |\n");
   printf("|   Manages candidates in a double-linked list, supporting   |\n");
   printf("|      assessment with justification and A-F grading.        |\n");
}

/* Print the main application menu */
void printMenu(void){
   printf("+============================================================+\n");
   printf("|  Tasks: T1=15p  T2=15p  T3=20p  T4=25p  T5=25p  (100p)     |\n");
   printf("+============================================================+\n");
   printf("|  1. Add candidate manually                                 |\n");
   printf("|  2. Load candidates from file                              |\n");
   printf("|  3. Assess a task (set points + justification)             |\n");
   printf("|  4. Print all candidates with scores and grades            |\n");
   printf("|  5. Print candidates by grade                              |\n");
   printf("|  6. Print ungraded candidates                              |\n");
   printf("|  7. Print full detail for a candidate                      |\n");
   printf("|  8. Randomly grade all candidates                          |\n");
   printf("|  0. Exit                                                   |\n");
   printf("+============================================================+\n");
   printf("  Choice: ");
}

