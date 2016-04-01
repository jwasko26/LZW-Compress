#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "SmartAlloc.h"
#include "LZWCmp.h"

static void CreateTree(LZWCmp **cmp, TreeNode **root, UInt element);
static void FreeTree(TreeNode **root);
static int TraverseTree(LZWCmp **cmp, TreeNode **cur);
static void DisplayTree(LZWCmp *cmp, TreeNode *root);
static LZWCmp *CompressBits(LZWCmp *cmp);

static TreeNode *FreeList = NULL;
static int StringSize = STRING_SIZE;
static int Flag = FALSE;
static int Bflag = FALSE;
static int Wflag = FALSE;

void LZWCmpInit(LZWCmp *cmp, CodeSink sink, void *sinkState, int recycleCode,
 int traceFlags) {
   
   UInt index = 0;
   
   cmp->root = NULL;
   cmp->sink = sink;
   cmp->sinkState = sinkState;
   cmp->numBits = BITS_PER_BYTE + 1;
   cmp->maxCode = (1 << cmp->numBits) - 1;
   cmp->recycleCode = recycleCode;
   cmp->nextInt = 0;
   cmp->bitsUsed = 0;
   cmp->pCode.data = malloc(StringSize * sizeof(char));
   cmp->pCode.size = 0;
   cmp->pCodeLimit = StringSize - 1;
   cmp->traceFlags = traceFlags;
   cmp->cst = CreateCodeSet(recycleCode + 1);
   while (index < NUM_SYMS) {
      CreateTree(&cmp, &cmp->root, index++);
   }
   NewCode(cmp->cst, -1);
   cmp->curLoc = cmp->root;
}

static void CreateTree(LZWCmp **cmp, TreeNode **root, UInt element) {
   if (!*root) {
      if (FreeList) {
         *root = FreeList;
         FreeList = (*root)->right;
      }
      else
         *root = malloc(sizeof(TreeNode));
      (*root)->left = NULL;
      (*root)->right = NULL;
      (*root)->cNum = element;
      NewCode((*cmp)->cst, element);
      return;
   }
   if ((*root)->cNum < element)
      CreateTree(cmp, &(*root)->right, element);
   else
      CreateTree(cmp, &(*root)->left, element);
}

static LZWCmp *CompressBits(LZWCmp *cmp) {
   if ((BITS_PER_INT - cmp->bitsUsed) >= cmp->numBits) {
      cmp->bitsUsed += cmp->numBits;
      cmp->nextInt |= cmp->curLoc->cNum << (BITS_PER_INT - cmp->bitsUsed);
   }
   else if ((BITS_PER_INT - cmp->bitsUsed) < cmp->numBits) {
      cmp->nextInt |= cmp->curLoc->cNum >> 
       (cmp->numBits - (BITS_PER_INT - cmp->bitsUsed));
      cmp->sink(cmp->sinkState, cmp->nextInt, FALSE);
      cmp->nextInt = 0;
      cmp->nextInt |= cmp->curLoc->cNum << 
       (BITS_PER_INT - (cmp->numBits - (BITS_PER_INT - cmp->bitsUsed)));
      cmp->bitsUsed = cmp->numBits - (BITS_PER_INT - cmp->bitsUsed);
   }
   return cmp;
}

void LZWCmpEncode(LZWCmp *cmp, UChar sym) {
   char *temp;
   int i;

   if (cmp->pCode.size == StringSize - 1) {
      StringSize *= 2;
      cmp->pCode.data = realloc(cmp->pCode.data, StringSize);
   }
   cmp->pCode.data[cmp->pCode.size++] = sym;
   cmp->pCode.data[cmp->pCode.size] = '\0';
   if (TraverseTree(&cmp, &cmp->curLoc)) {
      if (cmp->traceFlags & TRACE_CODES) {
         printf("Sending code %d\n", cmp->curLoc->cNum);
      }
      cmp = CompressBits(cmp);
      if (cmp->traceFlags & TRACE_TREE) {
         DisplayTree(cmp, cmp->root);
         printf("|\n\n");
      }
      if (Flag) {
         if (cmp->traceFlags & TRACE_RECYCLES)
            printf("Recycling dictionary...\n");
         Flag = FALSE;
         DestroyCodeSet(cmp->cst);
         FreeTree(&cmp->root);
         i = 0;
         cmp->root = NULL;
         cmp->numBits = BITS_PER_BYTE + 1;
         cmp->maxCode = (1 << cmp->numBits) - 1;
         cmp->cst = CreateCodeSet(cmp->recycleCode + 1);
         while (i < NUM_SYMS) {
            CreateTree(&cmp, &cmp->root, i++);
         }
         NewCode(cmp->cst, -1);
         cmp->curLoc = cmp->root;
      }
      if (Bflag) {
         Bflag = FALSE;
         cmp->numBits++;
         cmp->maxCode = (1 << cmp->numBits) - 1;
         if (cmp->traceFlags & TRACE_BUMPS)
            printf("Bump numBits to %d\n", cmp->numBits);
      }
      cmp->pCode.data[0] = cmp->pCode.data[cmp->pCode.size - 1];
      cmp->pCode.size = 0;
      cmp->pCode.data[++cmp->pCode.size] = '\0';
      cmp->curLoc = cmp->root;
      TraverseTree(&cmp, &cmp->curLoc);
   }
   Wflag = TRUE;
}

static int TraverseTree(LZWCmp **cmp, TreeNode **cur) {
   int i;

   if (!*cur) {
      if (FreeList) {
         *cur = FreeList;
         FreeList = (*cur)->right;
      }
      else
         *cur = malloc(sizeof(TreeNode));
      (*cur)->right = NULL;
      (*cur)->left = NULL;
      (*cur)->cNum = ExtendCode((*cmp)->cst, (*cmp)->curLoc->cNum);
      SetSuffix((*cmp)->cst, (*cur)->cNum, 
       (*cmp)->pCode.data[(*cmp)->pCode.size - 1]);
      if ((*cur)->cNum == (*cmp)->maxCode + 1)
         Bflag = TRUE;
      if ((*cur)->cNum == (*cmp)->recycleCode) {
         Bflag = FALSE;
         Flag = TRUE;
      }
      return 1;
   }
   (*cmp)->curCode = GetCode((*cmp)->cst, (*cur)->cNum);
   i = memcmp((*cmp)->curCode.data, (*cmp)->pCode.data, (*cmp)->pCode.size);
   if (i == 0 && (*cmp)->pCode.size != (*cmp)->curCode.size) {
      FreeCode((*cmp)->cst, (*cur)->cNum);
      return TraverseTree(cmp, &(*cur)->right);
   }
   FreeCode((*cmp)->cst, (*cur)->cNum);
   if (i == 0) {
      (*cmp)->curLoc = *cur;
      return 0;
   }
   else if (i < 0)
      return TraverseTree(cmp, &(*cur)->right);
   else
      return TraverseTree(cmp, &(*cur)->left);
}

static void DisplayTree(LZWCmp *cmp, TreeNode *root) {
   int i = 0;
   UChar *data;
   Code temp;

   if (!root)
      return;
   DisplayTree(cmp, root->left);
   temp = GetCode(cmp->cst, root->cNum);
   data = (UChar *)temp.data;
   printf("|");
   while (i < temp.size) {
      printf("%d", data[i]);
      if (i + 1 != temp.size)
         printf(" ");
      i++;
   }
   FreeCode(cmp->cst, root->cNum);
   DisplayTree(cmp, root->right);

}

void LZWCmpStop(LZWCmp *cmp) {
   if (Wflag) {
      if (cmp->traceFlags & TRACE_CODES)
         printf("Sending code %d\n", cmp->curLoc->cNum);
      if (cmp->traceFlags & TRACE_TREE) {
         DisplayTree(cmp, cmp->root);
         printf("|\n\n");
      }
      cmp = CompressBits(cmp);
   }
   if (cmp->traceFlags & TRACE_CODES)
      printf("Sending code %d\n", NUM_SYMS);
   if (cmp->traceFlags & TRACE_TREE) {
      DisplayTree(cmp, cmp->root);
      printf("|\n\n");
   }
   if ((BITS_PER_INT - cmp->bitsUsed) >= cmp->numBits) {
      cmp->bitsUsed += cmp->numBits;
      cmp->nextInt |= NUM_SYMS << (BITS_PER_INT - cmp->bitsUsed);
   }
   else if ((BITS_PER_INT - cmp->bitsUsed) < cmp->numBits) {
      cmp->nextInt |= NUM_SYMS >> 
       (cmp->numBits - (BITS_PER_INT - cmp->bitsUsed));
      cmp->sink(cmp->sinkState, cmp->nextInt, FALSE);
      cmp->nextInt = 0;
      cmp->nextInt |= NUM_SYMS << 
       (BITS_PER_INT - (cmp->numBits - (BITS_PER_INT - cmp->bitsUsed)));
      cmp->bitsUsed = cmp->numBits - (BITS_PER_INT - cmp->bitsUsed);
   }
   cmp->sink(cmp->sinkState, cmp->nextInt, FALSE);
   cmp->sink(cmp->sinkState, -1, TRUE);
}

void LZWCmpDestruct(LZWCmp *cmp) {
   DestroyCodeSet(cmp->cst);
   FreeTree(&cmp->root);
   free(cmp->pCode.data);
}

static void FreeTree(TreeNode **root) {
   if (*root) {
      FreeTree(&(*root)->left);
      FreeTree(&(*root)->right);
      (*root)->left = NULL;
      (*root)->right = NULL;
      if (FreeList)
         (*root)->right = FreeList;
      FreeList = *root;
   }
}

void LZWCmpClearFreelist() {
   TreeNode *temp;
  
   while (FreeList) {
      temp = FreeList;
      FreeList = temp->right;
      free(temp);
   }
}
