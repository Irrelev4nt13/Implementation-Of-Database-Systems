#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
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

SHT_Record Create_SHTRecord(char *name, int block)
{
  SHT_Record record;
  record.block = block;
  memcpy(record.name, name, strlen(name) + 1);
  return record;
}

int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char *fileName)
{
  // Initialize local variables
  int fileDesc;
  int headerId;

  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Create file with the given name
  CALL_OR_DIE(BF_CreateFile(sfileName));
  CALL_OR_DIE(BF_OpenFile(sfileName, &fileDesc));

  // Allocate Block
  CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));

  // Get the number of the blocks in order to save the header block id for later usage
  CALL_OR_DIE(BF_GetBlockCounter(fileDesc, &headerId));

  // Get a pointer in the beggining of the block
  void *data = BF_Block_GetData(block);

  // Write the metadata of the file
  // Initialize the required number of buckets with -1, which means that the current bucket is not
  // Mapped with any block
  SHT_info *info = data;
  info->fileDesc = fileDesc;
  info->numBuckets = buckets;
  info->headerBlock = --headerId;
  info->type_file = 's';
  info->numBlocks = buckets;
  memcpy(&info->filename, fileName, strlen(fileName));
  memcpy(&info->sfilename, sfileName, strlen(sfileName));
  for (int i = 0; i < buckets; i++)
    info->hashTable[i] = -1;
  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));

  BF_Block_Destroy(&block);
  return 0;
}

SHT_info *SHT_OpenSecondaryIndex(char *indexName)
{
  // Initialize temporary variables
  int fileDesc;
  void *data;

  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Open the file and pass the file descriptor in the local variable
  CALL_OR_DIE(BF_OpenFile(indexName, &fileDesc));
  CALL_OR_DIE(BF_GetBlock(fileDesc, 0, block));

  // We are getting a pointer in the beggining of the file
  // We have make a assumption that the first byte of the file with has it's type
  // 't' means hash file 'p' means heap file and 's' means secondary hash file
  // Checking if the current file matches the prescription of hash file and then procedding
  // By getting a pointer in the beginning of the data and return it
  data = BF_Block_GetData(block);
  char *type_file = data;
  if (type_file[0] != 's')
  {
    printf("Not a secondary hash file\n");
    return NULL;
  }
  SHT_info *info = data;

  BF_Block_Destroy(&block);
  return info;
}

int SHT_CloseSecondaryIndex(SHT_info *SHT_info)
{
  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Get the header block and unpin it because we are going to close the file
  CALL_OR_DIE(BF_GetBlock(SHT_info->fileDesc, 0, block));
  CALL_OR_DIE(BF_UnpinBlock(block));

  // Destroy block and close the file
  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(SHT_info->fileDesc));

  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info *sht_info, Record record, int block_id)
{
  // Initialize Block
  BF_Block *block;
  BF_Block_Init(&block);

  // Call hash function with argument the name and get a key back in
  int index = hash(record.name, sht_info->numBuckets);

  void *data;
  SHT_block_info *block_info;
  int local_id;

  // Check if the current bucket is mapped with a block
  // If not then allocate a new block store the metadata in the block
  // Also map the bucket with the new block
  if (sht_info->hashTable[index] == -1)
  {
    CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(SHT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE - sizeof(SHT_block_info)) / sizeof(SHT_Record);
    block_info->nextBF_Block = -1;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_GetBlockCounter(sht_info->fileDesc, &local_id));
    sht_info->hashTable[index] = --local_id;
  }
  else
  {
    // If it is mapped with a block just open the block and get two pointers in specific bytes
    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->hashTable[index], block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(SHT_block_info));
  }
  // If the block has space then copy the record
  // Increase the total number of records in the current block
  // Also make it dirty in order for to pass in the file
  SHT_Record *rec, infos = Create_SHTRecord(record.name, block_id);
  if (block_info->numRecords < block_info->maxRecords)
  {
    rec = data;
    memcpy(&rec[block_info->numRecords], &infos, sizeof(SHT_Record));
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
    CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, block));
    data = BF_Block_GetData(block);
    block_info = data + (BF_BLOCK_SIZE - sizeof(SHT_block_info));
    block_info->numRecords = 0;
    block_info->maxRecords = (BF_BLOCK_SIZE - sizeof(SHT_block_info)) / sizeof(SHT_Record);
    block_info->nextBF_Block = sht_info->hashTable[index];

    // Copy the record
    // Increase the total number of records in the current block
    // Also make it dirty in order for to pass in the file
    rec = data;
    memcpy(&rec[block_info->numRecords], &infos, sizeof(SHT_Record));
    block_info->numRecords++;
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));

    // Get the header block because we need to change the hashtable and few other variables
    // The total number of blocks that being used by the buckets, which means
    // The header block is not counted
    // And also map the new block with the current bucket
    // Make it dirty in order for to pass in the file
    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->headerBlock, block));
    data = BF_Block_GetData(block);
    sht_info = data;
    sht_info->numBlocks++;
    CALL_OR_DIE(BF_GetBlockCounter(sht_info->fileDesc, &local_id));
    sht_info->hashTable[index] = --local_id;
    BF_Block_SetDirty(block);
  }
  BF_Block_Destroy(&block);
  return 0;
}

int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info, char *name)
{
  BF_Block *block, *block1;
  BF_Block_Init(&block);
  BF_Block_Init(&block1);

  int index = hash(name, sht_info->numBuckets);

  int blockCounter = 0;
  int curr_blockId = sht_info->hashTable[index];
  int block_num;
  BF_GetBlockCounter(ht_info->fileDesc, &block_num);
  int *visited = malloc(block_num * sizeof(int)); /* Array to help us print all Records needed without duplicates */
  for (int i = 0; i < block_num; i++)
    visited[i] = 0; /* Each element of the array is set to 0 */

  while (curr_blockId != -1)
  {
    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, curr_blockId, block));
    void *data = BF_Block_GetData(block);
    SHT_block_info *block_info = data + (BF_BLOCK_SIZE - sizeof(SHT_block_info));

    SHT_Record *rec = data;
    for (int i = 0; i < block_info->numRecords; i++)
    {
      if (visited[rec[i].block] == 0)                                      /* We have not printed the Records with record.name==name in this block */
      {                                                                    /* If visited[rec[i].block] is not zero then we have already printed all the records we need which are stored in the block */
        CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, rec[i].block, block1)); /* Get the block which is stored Records with the record.name "name "name" */

        void *data1 = BF_Block_GetData(block1);
        HT_block_info *block_info1 = data1 + (BF_BLOCK_SIZE - sizeof(HT_block_info));

        Record *rec1 = data1;
        for (int i = 0; i < block_info1->numRecords; i++)
        {                                      /* Traverse all the Records of the block rec[i].block */
          if (strcmp(name, rec1[i].name) == 0) /* If the record has the field name we are looking for, print it */
          {
            printRecord(rec1[i]);
            blockCounter++;
          }
        }
        visited[rec[i].block] = 1; /* We have printed all the needed records of this block so dont visit it again! */
        CALL_OR_DIE(BF_UnpinBlock(block1));
      }
    }
    curr_blockId = block_info->nextBF_Block;
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  free(visited);
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&block1);
  return blockCounter;
}
