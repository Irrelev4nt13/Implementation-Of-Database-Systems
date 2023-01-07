#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

// A simple hash function that takes a string and returns a value after several calculations
int hash(const char *key, const int buckets)
{
  long int value = 0;
  int i = 0;
  int key_len = strlen(key);

  for (; i < key_len; ++i)
    value = value * 37 + key[i];
  value = value % buckets;

  return value;
}

int HT_CreateFile(char *fileName, int buckets)
{
  // Initialize local variables 
  int fileDesc;
  int headerId;

  // Initialize Block
  BF_Block *block;  
  BF_Block_Init(&block);

  // Create file with the given name
  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));
  
  // Allocate Block
  CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));
  
  // Get the number of the blocks in order to save the header block id for later usage
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &headerId));

  // Get a pointer in the beggining of the block
  void *data = BF_Block_GetData(block);
  
  // Write the metadata of the file 
  // Initialize the required number of buckets with -1, which means that the current bucket is not
  // Mapped with any block
  HT_info *info = data;
  info->fileDesc = fileDesc;
  info->numBuckets = buckets;
  info->headerBlock = --headerId;
  info->type_file = 't';
  info->numBlocks = buckets;
  for (int i = 0; i < buckets; i++)
    info->hashTable[i] = -1;
  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);
  return 0;
}

HT_info *HT_OpenFile(char *fileName)
{
  // Initialize temporary variables
  int fileDesc;
  void *data;

  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Open the file and pass the file descriptor in the local variable 
  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));
  CALL_OR_DIE(BF_GetBlock(fileDesc, 0, block));

  // We are getting a pointer in the beggining of the file
  // We have make a assumption that the first byte of the file with has it's type
  // 't' means hash file 'p' means heap file and 's' means secondary hash file
  // Checking if the current file matches the prescription of hash file and then procedding
  // By getting a pointer in the beginning of the data and return it
  data = BF_Block_GetData(block);
  char *type_file = data;
  if (type_file[0] != 't')
  {
    printf("Not a hash file\n");
    return NULL;
  }
  HT_info *info = data;

  BF_Block_Destroy(&block);
  return info;
}

int HT_CloseFile(HT_info *HT_info)
{
  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Get the header block and unpin it because we are going to close the file
  CALL_OR_DIE(BF_GetBlock(HT_info->fileDesc, 0, block));
  CALL_OR_DIE(BF_UnpinBlock(block));

  // Destroy block and close the file
  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(HT_info->fileDesc));

  return 0;
}

int HT_InsertEntry(HT_info *ht_info, Record record)
{
  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Convert record id to string and pash it to hash function in order to get a key back
  char intToString[20];
  sprintf(intToString, "%d", record.id);
  int index = hash(intToString, ht_info->numBuckets);
  
  void* data;
  HT_block_info* block_info;
  int blockId;

  Record *rec;
  // Check if the current bucket is mapped with a block
  // If not then allocate a new block store the metadata in the block
  // Also map the bucket with the new block
  if (ht_info->hashTable[index] == -1) 
  {
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(HT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
    block_info->nextBF_Block = -1;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_GetBlockCounter(ht_info->fileDesc, &blockId));
    ht_info->hashTable[index] = --blockId;
  }
  else
  {
    // If it is mapped with a block just open the block and get two pointers in specific bytes
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->hashTable[index], block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(HT_block_info));
  }
  // If the block has space then copy the record
  // Increase the total number of records in the current block
  // Also make it dirty in order for to pass in the file
  if (block_info->numRecords < block_info->maxRecords)
  {
    rec = data;
    memcpy(&rec[block_info->numRecords], &record, sizeof(Record));
    block_info->numRecords++;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  else
  {
    // If the block does not have the required space then unpin it because we dont need it anymore
    // Allocate a new block store the metadata in the block
    // Also link the new block with the previous one
    CALL_OR_DIE(BF_UnpinBlock(block));
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(HT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE - sizeof(HT_block_info)) / sizeof(Record);
    block_info->nextBF_Block = ht_info->hashTable[index];

    // Copy the record
    // Increase the total number of records in the current block
    // Also make it dirty in order for to pass in the file
    rec = data;
    memcpy(&rec[0], &record, sizeof(Record));
    block_info->numRecords++;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

    // Get the header block because we need to change the hashtable and few other variables
    // The total number of blocks that being used by the buckets, which means
    // The header block is not counted
    // And also map the new block with the current bucket
    // Make it dirty in order for to pass in the file
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->headerBlock, block));
    data = BF_Block_GetData(block);
    ht_info = data;
    ht_info->numBlocks++;
    ht_info->hashTable[index] = ht_info->numBlocks;
    BF_Block_SetDirty(block);
  }
  BF_Block_Destroy(&block);
  return ht_info->hashTable[index];
}

int HT_GetAllEntries(HT_info *ht_info, int value)
{
  // Initialize block
  BF_Block *block;
  BF_Block_Init(&block);

  // Convert record id to string and pash it to hash function in order to get the key
  char intToString[20];
  sprintf(intToString, "%d", value);
  int index = hash(intToString, ht_info->numBuckets);

  // Keep track of the number of blocks that we will parsh in order to find all the records
  int blockCounter = 0;
  // Initialize the search with the last block and go backwards
  int curr_blockId = ht_info->hashTable[index];
  while (curr_blockId != -1)
  {
    // Get the block from the memory
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, curr_blockId, block));
    void *data = BF_Block_GetData(block);
    // We got a new block so we increase the counter
    blockCounter++;
    // Get the metadata of the block
    HT_block_info *block_info = data + (BF_BLOCK_SIZE - sizeof(HT_block_info));

    Record *rec = data;
    // Parse all of the records
    // If the id matches the value then print it
    for (int i = 0; i < block_info->numRecords; i++)
    {
      if (rec[i].id == value)
        printRecord(rec[i]);
    }
    // Then we go to block that we had before
    curr_blockId = block_info->nextBF_Block;
    // Unpin the current one because we don't need it anymore
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return blockCounter;
}
