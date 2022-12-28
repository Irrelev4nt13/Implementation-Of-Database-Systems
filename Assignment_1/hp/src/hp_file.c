#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/bf.h"
#include "../include/hp_file.h"
#include "../include/record.h"

int HP_CreateFile(char *fileName)
{
  CALL_BF(BF_CreateFile(fileName));
  return BF_OK;
}

HP_info *HP_OpenFile(char *fileName)
{
  int fd1;
  BF_Block *block;
  void *data;

  BF_Block_Init(&block);
  CALL_BF(BF_OpenFile(fileName, &fd1));
  CALL_BF(BF_AllocateBlock(fd1, block));
  data = BF_Block_GetData(block);
  HP_info *info = data;
  info->located = block;
  info->fileDesc = fd1;
  info->max = (BF_BLOCK_SIZE - sizeof(HP_block_info)) / sizeof(Record);
  info->last_id = 0;
  BF_Block_SetDirty(block);
  return info;
}

int HP_CloseFile(HP_info *hp_info)
{
  BF_UnpinBlock(hp_info->located);
  CALL_BF(BF_CloseFile(hp_info->fileDesc));
  return 0;
}

int HP_InsertEntry(HP_info *hp_info, Record record)
{
  BF_Block *block;
  void *data;
  int block_num;

  BF_Block_Init(&block);
  BF_GetBlockCounter(hp_info->fileDesc, &block_num);
  if (block_num > 1)
  {
    CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->last_id, block));
    data = BF_Block_GetData(block);
    HP_block_info *blinfo = data + hp_info->max * sizeof(Record);
    if (blinfo->rec_num < hp_info->max)
    {
      /* Add to an existing block which is not full */
      Record *rec = data;
      rec[blinfo->rec_num] = record;
      blinfo->rec_num++;
      BF_Block_SetDirty(block);
      BF_UnpinBlock(block);
      BF_Block_Destroy(&block);
      return 0;
    }
  }

  /* Make a new block either because we have 0 either because the others are full */
  // printf("%p  %p\n", block, hp_info->located);
  CALL_BF(BF_AllocateBlock(hp_info->fileDesc, block));
  data = BF_Block_GetData(block);
  HP_block_info *blinfo = data + hp_info->max * sizeof(Record);
  Record *rec = data;
  rec[0] = record;
  blinfo->rec_num = 1;
  hp_info->last_id = block_num;
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return 0;
}

int HP_GetAllEntries(HP_info *hp_info, int value)
{
  return 0;
}
