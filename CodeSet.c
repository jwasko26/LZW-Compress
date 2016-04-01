#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "SmartAlloc.h"
#include "CodeSet.h"

typedef struct CodeSet {
   int size;
   int index;
   struct CodeEntry *array;
} CodeSet;

typedef struct CodeEntry {
   char freed;
   char contin;
   char numcalls;
   char *code;
   char curbyte;
   struct CodeEntry *prefix;
} CodeEntry;

void *CreateCodeSet(int numCodes) {
   CodeSet *rtn = malloc(sizeof(CodeSet));
   
   rtn->size = numCodes;
   rtn->index = 0;
   rtn->array = malloc(numCodes * sizeof(CodeEntry));
   return rtn;
}

int NewCode(void *codeSet, char val) {
   CodeSet *temp = (CodeSet *)codeSet;
   CodeEntry *temp2 = (CodeEntry *)temp->array;

   temp2[temp->index].curbyte = val;
   temp2[temp->index].contin = -1;
   temp2[temp->index].freed = 1;
   temp2[temp->index].numcalls = 0;
   return temp->index++;
}

int ExtendCode(void *codeSet, int oldCode) {
   CodeSet *temp = (CodeSet *)codeSet;
   CodeEntry *temp2 = (CodeEntry *)temp->array;

   temp2[temp->index].prefix = &temp2[oldCode];
   temp2[temp->index].numcalls = 0;
   temp2[temp->index].curbyte = 0;
   return temp->index++;
}

void SetSuffix(void *codeSet, int code, char suffix) {
   CodeSet *temp = (CodeSet *)codeSet;
   CodeEntry *temp2 = (CodeEntry *)temp->array;

   temp2[code].curbyte = suffix;
}

Code GetCode(void *codeSet, int code) {
   Code temp;
   int size = 1;
   int i;
   CodeEntry link;
   CodeEntry temp4;
   CodeSet *temp2 = (CodeSet *)codeSet;
   CodeEntry *temp3 = (CodeEntry *)temp2->array;

   link = temp3[code];

   while (link.contin != -1) {
      size++;
      link = *(link.prefix);
   }
   if (temp3[code].freed != -1) {
      temp3[code].code = malloc(size + 1);
      temp3[code].freed = -1;
   }
   temp3[code].code[size] = '\0';
   temp.size = size;
   i = size - 1;
   temp4 = temp3[code];
   while (temp4.contin != -1) {
      temp3[code].code[i] = temp4.curbyte;
      i--;
      temp4 = *(temp4.prefix);
   }
   temp3[code].numcalls++;
   temp3[code].code[i] = temp4.curbyte;
   temp.data = (unsigned char *)temp3[code].code;
   return temp;
}

void FreeCode(void *codeSet, int code) {   
   CodeSet *temp = (CodeSet *)codeSet;
   CodeEntry *temp2 = (CodeEntry *)temp->array;

   if (temp2[code].freed == -1) {
      if (temp2[code].numcalls == 1) {
         temp2[code].freed = 1;
         free(temp2[code].code);
      }
      temp2[code].numcalls--;
   }
} 

void DestroyCodeSet(void *codeSet) {
   CodeSet *temp = (CodeSet *)codeSet;
   CodeEntry *temp2 = (CodeEntry *)temp->array;
   int i;
   
   for (i = 0; i < temp->size; i++) {
      if (temp2[i].freed == -1) {
         temp2[i].freed == 1;
         free(temp2[i].code);
      }
   }
   free(temp->array);
   free(temp);
}
