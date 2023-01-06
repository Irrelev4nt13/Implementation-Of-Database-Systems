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

int hash(const char *key, const int buckets) {
  long int value = 0;
  int i = 0;
  int key_len = strlen(key);

  for (; i < key_len; ++i)
    value = value * 37 + key[i];
  value = value % buckets;

  return value;
}

int HT_CreateFile(char* fileName, int buckets) {

  int fileDesc;
  int headerId; 

  BF_Block* block;
  BF_Block_Init(&block);

  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));

  CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));

  CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &headerId));
  
  void* data = BF_Block_GetData(block);
  HT_info *info = data;
  info->fileDesc = fileDesc;
  info->numBuckets = buckets;
  info->headerBlock = --headerId;
  info->isHeapFile = false;
  info->isHashFile = true;
  info->numBlocks = buckets;
  for (int i = 0; i < buckets; i++)
    info->hashTable[i] = i+1;
  BF_Block_SetDirty(block);  
  CALL_OR_DIE(BF_UnpinBlock(block));

  HT_block_info *block_info = NULL;
  for (int i = 0; i < buckets; i++) {
    CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE-sizeof(HT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE-sizeof(HT_block_info))/sizeof(Record);
    block_info->nextBF_Block = -1;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);
  // CALL_OR_DIE(BF_CloseFile(fileDesc));
  return 0;
}

HT_info* HT_OpenFile(char *fileName) {

  int fileDesc;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);

  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));

  CALL_OR_DIE(BF_GetBlock(fileDesc, 0, block));

  data = BF_Block_GetData(block);
  HT_info* info = data;
  if (info->isHashFile != true || info->isHeapFile != false) {
    printf("Not a hash file\n");
    return NULL;
  }

  // CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return info;
}


int HT_CloseFile( HT_info* HT_info ) {

  BF_Block* block;
  BF_Block_Init(&block);
  
  CALL_OR_DIE(BF_GetBlock(HT_info->fileDesc, 0, block));
  CALL_OR_DIE(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(HT_info->fileDesc));
  
  return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record) {

  BF_Block* block;
  BF_Block_Init(&block);
  
  char intToString[20];
  sprintf(intToString,"%d",record.id);
  int index = hash(intToString, ht_info->numBuckets);

  CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->hashTable[index], block));
  void* data = BF_Block_GetData(block);
  HT_block_info *block_info = data + (BF_BLOCK_SIZE-sizeof(HT_block_info));
  Record* rec;

  if (block_info->numRecords < block_info->maxRecords) {
    rec = data;
    memcpy(&rec[block_info->numRecords], &record, sizeof(Record));
    block_info->numRecords++;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  else {
    CALL_OR_DIE(BF_UnpinBlock(block));
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE-sizeof(HT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE-sizeof(HT_block_info))/sizeof(Record);
    block_info->nextBF_Block = ht_info->hashTable[index];

    rec = data;
    memcpy(&rec[0], &record, sizeof(Record));
    block_info->numRecords++;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->headerBlock, block));
    data = BF_Block_GetData(block);
    ht_info = data;
    ht_info->numBlocks++;
    ht_info->hashTable[index] = ht_info->numBlocks;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return ht_info->hashTable[index];
}

int HT_GetAllEntries(HT_info* ht_info, int value ) {
  
  BF_Block* block;
  BF_Block_Init(&block);

  char intToString[20];
  sprintf(intToString,"%d",value);
  int index = hash(intToString, ht_info->numBuckets);

  int blockCounter = 0;
  int curr_blockId = ht_info->hashTable[index];
  bool found = false;
  while(curr_blockId != -1) { 
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, curr_blockId, block));
    void* data = BF_Block_GetData(block);
    blockCounter++;
    HT_block_info *block_info = data + (BF_BLOCK_SIZE-sizeof(HT_block_info));
    
    Record* rec = data;
    for (int i = 0; i < block_info->numRecords; i++) {  
      if(rec[i].id == value) {
        printRecord(rec[i]);
        found = true;
        break;
      }  
    }
    if(found == true)
      break;
    curr_blockId = block_info->nextBF_Block;
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return blockCounter;
}




