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

unsigned int hash(const char *key, const int buckets) {
  unsigned long int value = 0;
  unsigned int i = 0;
  unsigned int key_len = strlen(key);

  // do several rounds of multiplication
  for (; i < key_len; ++i) {
      value = value * 37 + key[i];
  }

  // make sure value is 0 <= value < TABLE_SIZE
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

  // printf("%ld\n", (BF_BLOCK_SIZE-sizeof(HT_block_info))/sizeof(Record));

  // printf("%ld\n", sizeof(HT_block_info));

  HT_block_info *block_info = NULL;
  for (int i = 0; i < buckets; i++) {
    CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data; //+ (BF_BLOCK_SIZE-sizeof(HT_block_info));
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
  // printRecord(record);
  BF_Block* block;
  BF_Block_Init(&block);
  
  char intToString[20];
  sprintf(intToString,"%d",record.id);
  int index = hash(intToString, ht_info->numBuckets);
  // index = 1;
  // printf("index %d %ld\n", index, ht_info->hashTable[index]);
  

  CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->hashTable[index], block));
  void* data = BF_Block_GetData(block);
  HT_block_info *block_info = data;
  // printf("number of records %ld\n", block_info->numRecords);
  // printf("max number of records %ld\n", block_info->maxRecords);
  
  // printf("%d\n", block_info->nextBF_Block);

  if (block_info->numRecords < block_info->maxRecords) {
    Record* rec = data + sizeof(HT_block_info) + (sizeof(Record) * (block_info->numRecords));
    memcpy(&rec[0], &record, sizeof(Record));
    block_info->numRecords++;
    // printRecord(rec[0]);
    // rec = data + sizeof(HT_block_info) + (sizeof(Record) * (block_info->numRecords));
    // record = randomRecord();
    // memcpy(&rec[0], &record, sizeof(Record));
    // block_info->numRecords++;
    // printRecord(rec[0]);
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  else {
    CALL_OR_DIE(BF_UnpinBlock(block));
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data;
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE-sizeof(HT_block_info))/sizeof(Record);
    block_info->nextBF_Block = index;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

    Record* rec = data + sizeof(HT_block_info) + (sizeof(Record) * block_info->numRecords);
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

  int blockCounter = 1;
  CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->hashTable[0], block));
  void* data = BF_Block_GetData(block);
  HT_block_info *block_info = data;
  Record* rec = data + sizeof(HT_block_info);
  printf("number of records %ld\n", block_info->numRecords);
  for (int i = 0; i < block_info->numRecords; i++) {  
    int index = i * sizeof(Record);
    // printRecord(rec[0*(int)sizeof(Record)]);
    if(rec[index].id == value)
      printRecord(rec[index]);
  }
  // while(1) {
  //   int i =0;
  //   printRecord(rec[i]);
  //   i++;
  //   getchar();
  // }
  CALL_OR_DIE(BF_UnpinBlock(block));
  return blockCounter;
}




