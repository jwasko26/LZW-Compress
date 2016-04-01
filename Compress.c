#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "SmartAlloc.h"
#include "LZWCmp.h"
#include "MyLib.h"

#define SIZE_OF_ARG 2
#define FILE_SIZE 3
#define FORMAT_LENGTH 6
#define RECYCLE_SIZE 4096
#define TRACE_SPACE 0x10

typedef struct State {
   int count;
   FILE *fp;
} State;

static int ReportSpace = FALSE;

void CodeS(void *state, UInt code, int done) {
   State *tempS = state;
   
   if (!done) {
      if (tempS->fp) {

         fprintf(tempS->fp, "%08X", code);

         if (tempS->count != FORMAT_LENGTH + 1) {
            fprintf(tempS->fp, " ");
            tempS->count++;
         }
         else {
            tempS->count = 0;
            fprintf(tempS->fp, "\n");
         }
      }
   }
   else {
      if (tempS->count > 0)
         fprintf(tempS->fp, "\n");
   }
}

int GetFlags(int argc, char **argv) {
   int indexCount = 1, traceFlags = 0, flagCount;
   char *cFlags = calloc(FORMAT_LENGTH, sizeof(char)), *tempC;

   while (indexCount < argc) {
      if (*argv[indexCount] == '-') {
         if (strlen(argv[indexCount]) > SIZE_OF_ARG)
            strcpy(cFlags, argv[indexCount] + 1);
         else
            strcat(cFlags, argv[indexCount] + 1);
      }
      indexCount++;
   }
   tempC = cFlags;
   for (flagCount = 0; flagCount < strlen(cFlags); flagCount++) {
      if (*tempC == 't')
         traceFlags |= 1 << 0;
      else if (*tempC == 'b')
         traceFlags |= 1 << 1;
      else if (*tempC == 'r')
         traceFlags |= 1 << 2;
      else if (*tempC == 'c')
         traceFlags |= 1 << 2 + 1;
      else if (*tempC == 's')
         ReportSpace = TRUE;
      else
         printf("Bad argument: %c\n", *tempC);
      tempC++;
   }
   free(cFlags);
   return traceFlags;
}

int main(int argc, char *argv[]) {
   int ndxCount = 1, traceFlags = 0, loopCount = 0, flagCount, tmpInt;
   char *nameC, *tempC;
   int fIndex;
   unsigned long space;
   LZWCmp cmp;
   UInt ucharC;
   FILE *fp, *wp;
   State states;
   
   traceFlags = GetFlags(argc, argv);
   for (ndxCount = 1; ndxCount < argc && *argv[ndxCount] == '-'; ndxCount++)
      ;
   fIndex = argc - ndxCount;
   for (loopCount = 1; loopCount <= fIndex; loopCount++) {
      if (!(fp = fopen(argv[ndxCount], "r"))) {
         printf("Cannot open %s\n", argv[ndxCount]);
         return 1;
      }
      
      nameC = malloc(strlen(argv[ndxCount]) + FILE_SIZE);
      strcpy(nameC, argv[ndxCount]);
      ndxCount++;
      strcat(nameC, ".Z\0");
      wp = fopen(nameC, "w+");
      states.count = 0;
      states.fp = wp;
      LZWCmpInit(&cmp, CodeS, &states, RECYCLE_SIZE, traceFlags);
      
      while ((ucharC = fgetc(fp)) != EOF) {
         LZWCmpEncode(&cmp, ucharC);
      }
      LZWCmpStop(&cmp);
      if (ReportSpace) {
         tmpInt = strlen(nameC) - 2;
         space = report_space();
         printf("Space after LZWCmpStop for %.*s: %ld\n", tmpInt, nameC, space);
      }
      
      free(nameC);
      LZWCmpDestruct(&cmp);
      fclose(fp);
      fclose(wp);
   }     
   
   LZWCmpClearFreelist();
   if (ReportSpace)
      printf("Final space: %ld\n", report_space());
   
   return 0;
}
