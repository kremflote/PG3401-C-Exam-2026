/* --------------------------------------------------------------------------------------

 Project:       PG3401 Exam Task 3
 Author:        Candidate 230
 Description:   Menu-based exam grading system.
                Receives instructions from the user via keyboard input
                and calls all gradebook functions accordingly.

 Justification: Database operations in task3_database.c, display in task3_print.c, and
                menu/input handling in task3_main.c. Each file only exposes what other
                files need through task3.h

/*-------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/* Standard C libraries                                                                */
/* ------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------------------------*/
/* Include files                                                                       */
/* ------------------------------------------------------------------------------------*/
#include "task3.h"

/* ------------------------------------------------------------------------------------*/
/* Helpers                                                                             */
/* ------------------------------------------------------------------------------------*/

/* Read a trimmed string from stdin into szBuf (max iBufSize chars)   */
static void readLine(char *szBuf, int iBufSize){
   char *pNewline = NULL;
   memset(szBuf, 0, iBufSize); /* Defensive memset */
   if (fgets(szBuf, iBufSize, stdin) != NULL){
      pNewline = strchr(szBuf, '\n');
      if (pNewline != NULL){ /* Avoids dereferencing null causing crash     */
         *pNewline = '\0';
      }
   }
}

/* Menu handler: add a single candidate by manually entering an ID            */
static void handleAddCandidate(GRADEBOOK **ppGradebook){
   char szId[MAX_CANDIDATE_ID];
   printf("Enter candidate ID (4-digit): ");
   readLine(szId, sizeof(szId));
   AddCandidate(ppGradebook, szId);
}

/* Menu handler: load candidates from a file                                  */
static void handleLoadFromFile(GRADEBOOK **ppGradebook){
   char szFilename[BUFFER_SIZE];
   printf("Enter filename: ");
   readLine(szFilename, sizeof(szFilename));
   LoadCandidatesFromFile(ppGradebook, szFilename);
}

/* Menu handler: assess one task for one candidate                            */
static void handleAssessTask(GRADEBOOK **ppGradebook){
   char szId[MAX_CANDIDATE_ID];
   char szJust[MAX_JUSTIFICATION];
   char szBuf[BUFFER_SIZE];
   char *pEnd = NULL;
   long lTask = 0;
   long lPoints = 0;

   printf("Candidate ID: ");
   readLine(szId, sizeof(szId));

   printf("Task number (1-%d) : ", NUM_TASKS);
   readLine(szBuf, sizeof(szBuf));
   lTask = strtol(szBuf, &pEnd, 10);
   if (pEnd == szBuf){
      printf("Invalid task number.\n");
      return;
   }

   printf("Points awarded: ");
   readLine(szBuf, sizeof(szBuf));
   lPoints = strtol(szBuf, &pEnd, 10);
   if (pEnd == szBuf){
      printf("Invalid points.\n");
      return;
   }

   printf("Justification: ");
   readLine(szJust, sizeof(szJust));

   AssessTask(ppGradebook, szId, (int)lTask, (int)lPoints, szJust);
}

/* Menu handler: print all candidates by a specific grade                     */
static void handlePrintByGrade(GRADEBOOK **ppGradebook){
   char szBuf[BUFFER_SIZE];
   char cGrade = '\0';

   printf("Enter grade (A/B/C/D/E/F): ");
   readLine(szBuf, sizeof(szBuf));
   if (szBuf[0] == '\0'){
      printf("No grade entered.\n");
      return;
   }
   cGrade = toupper((unsigned char)szBuf[0]);

   if (cGrade < 'A' || cGrade > 'F'){
      printf("Invalid grade '%c'.\n", cGrade);
      return;
   }
   PrintByGrade(ppGradebook, cGrade);
}

/* Menu handler: print full detail for one candidate                          */
static void handlePrintDetail(GRADEBOOK **ppGradebook){
   char szId[MAX_CANDIDATE_ID];
   printf("Enter candidate ID: ");
   readLine(szId, sizeof(szId));
   PrintCandidateDetail(ppGradebook, szId);
}

/* ---------------------------------------------------------------------------*/
/* Main                                                                       */
/* ---------------------------------------------------------------------------*/
int main(void){
   GRADEBOOK *pGradebook = NULL;
   char szChoice[BUFFER_SIZE];
   int iChoice = -1;
   char *pEnd = NULL;

   printf("\nInitializing program... \n");
   printf("Creating gradebook... \n");

   if (CreateGradebook(&pGradebook) != 0)
      return EXIT_FAILURE;

   printf("Gradebook successfully created.\n");

   printHeader();

   do {
      printMenu();
      readLine(szChoice, sizeof(szChoice));
      iChoice = (int)strtol(szChoice, &pEnd, 10);

      if (pEnd == szChoice){
         printf("Please enter a number.\n");
         continue;
      }

      printf("\n");

      switch (iChoice){
         case 1: handleAddCandidate(&pGradebook);              break;
         case 2: handleLoadFromFile(&pGradebook);              break;
         case 3: handleAssessTask(&pGradebook);                break;
         case 4: PrintAllCandidates(&pGradebook);              break;
         case 5: handlePrintByGrade(&pGradebook);              break;
         case 6: PrintUngraded(&pGradebook);                   break;
         case 7: handlePrintDetail(&pGradebook);               break;
         case 8: RandomGradeAll(&pGradebook);                  break;
         case 0: printf("Exiting...\n");                       break;
         default: printf("Invalid choice. Enter 0-8.\n");      break;
      }

   } while (iChoice != 0);

   /* Clean up all allocated memory before exit */
   DestroyGradebook(&pGradebook);

   return EXIT_SUCCESS;
}

