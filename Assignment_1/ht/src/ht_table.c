#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int HT_CreateFile(char* fileName, int buckets) {

  // create and initialize a hash file with name fileName and the give number of buckets
  // create the header block which will has the informations of HT_info
  
  // bf_create
  // bf_open
  // bf allocate headerblock
  // get headerblock id
  // and the block itself
  // assign the values to HT_info
  // save the HT_info in the first block
  // write the block
  // create a hash table with buckets (many lines of code)
  // bf_close

  /* int fileDesc;
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));
  CALL_OR_DIE(BF_AllocateBlock(fileDesc, &block));

  int data = BF_Block_GetData(block); */

  return 0;
}

HT_info* HT_OpenFile(char *fileName) {
  // open
  // read first block
  // check that it is a hash file
  // return a copy of HT_info to return
  return NULL;
}


int HT_CloseFile( HT_info* HT_info ) {
  // bf_close
  // free the memory
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record) {
  return 0;
}

int HT_GetAllEntries(HT_info* ht_info, int value ) {
  return 0;
}




