#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/bf.h"
#include "../include/hp_file.h"
#include "../include/record.h"

int HP_CreateFile(char *fileName)
{
  CALL_BF(BF_CreateFile(fileName));
  int fd1;
  BF_Block *block;
  void *data;

  BF_Block_Init(&block);                 /* Initialize block */
  CALL_BF(BF_OpenFile(fileName, &fd1));  /* Open file */
  CALL_BF(BF_AllocateBlock(fd1, block)); /* Allocate memory for new block */
  data = BF_Block_GetData(block);
  HP_info *info = data;                                                 /* First block allocated will have the information about the whole file */
  info->fileDesc = fd1;                                                 /* Store the file disc */
  info->max = (BF_BLOCK_SIZE - sizeof(HP_block_info)) / sizeof(Record); /* Calculate the number of the maximum Records each block can have */
  info->last_id = 0;                                                    /* Initialize with 0 */
  info->type_file = 'p';                                                /* Store the type of the file */
  BF_Block_SetDirty(block);                                             /* We made changes so set the block dirty */

  CALL_BF(BF_UnpinBlock(block)); /* Unpin the block */
  BF_Block_Destroy(&block);      /* Destroy the block as we no longer need it */
  return BF_OK;
}

HP_info *HP_OpenFile(char *fileName)
{
  int fd1;
  BF_Block *block = NULL;
  void *data;

  BF_Block_Init(&block);

  CALL_BF(BF_OpenFile(fileName, &fd1));
  CALL_BF(BF_GetBlock(fd1, 0, block));
  data = BF_Block_GetData(block);
  char *type_file = data;  /* The first character stored in the file represents the type of the file */
  if (type_file[0] != 'p') /* Check if the file opened is heap file */
  {                        /* If not a heap file return error */
    printf("Not a heap file\n");
    return NULL;
  }
  HP_info *info = data;

  BF_Block_Destroy(&block);
  return info;
}

int HP_CloseFile(HP_info *hp_info)
{
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(hp_info->fileDesc, 0, block));
  CALL_BF(BF_UnpinBlock(block)); /* Unpin the block which contains the information of the file */
  BF_Block_Destroy(&block);
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
  {                                                                   /* If the file has some block allocated for storing, proceed to try inserting the record to one of these blocks */
    CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->last_id, block)); /* Get the last block we inserted a record to insert the new record */
    data = BF_Block_GetData(block);
    HP_block_info *blinfo = data + hp_info->max * sizeof(Record); /* Use the right offset to get the block data */
    if (blinfo->rec_num < hp_info->max)
    {
      /* Add to an existing block which is not full */
      Record *rec = data;
      memcpy(&rec[blinfo->rec_num], &record, sizeof(Record)); /* Store the record to the block */
      blinfo->rec_num++;                                      /* Increment the number of records stored */
      BF_Block_SetDirty(block);                               /* We made changes so we make the block dirty */
      CALL_BF(BF_UnpinBlock(block));                          /* Unpin the block */
      BF_Block_Destroy(&block);                               /* Destroy the block as we no longer need it */
      return hp_info->last_id;                                /* Return the id of the block we inserted */
    }
    CALL_BF(BF_UnpinBlock(block));
  }
  /* Make a new block either because we have 0 either because the others are full */
  CALL_BF(BF_AllocateBlock(hp_info->fileDesc, block));
  data = BF_Block_GetData(block);
  HP_block_info *blinfo = data + hp_info->max * sizeof(Record); /* Use the right offset to get the block data */
  Record *rec = data;
  memcpy(&rec[0], &record, sizeof(Record)); /* Store the record to the block */
  blinfo->rec_num = 1;                      /* We inserted the first record to the block so set the record counter for this block to 1 */
  hp_info->last_id = block_num;
  BF_Block_SetDirty(block);      /* We made changes so we make the block dirty */
  CALL_BF(BF_UnpinBlock(block)); /* Unpin the block */
  BF_Block_Destroy(&block);      /* Destroy the block as we no longer need it */
  return hp_info->last_id;       /* Return the id of the block we inserted */
}

int HP_GetAllEntries(HP_info *hp_info, int value)
{
  BF_Block *block;
  void *data;
  int block_num, counter = -1;
  CALL_BF(BF_GetBlockCounter(hp_info->fileDesc, &block_num));
  BF_Block_Init(&block);
  for (int i = 1; i < block_num; i++)
  {
    CALL_BF(BF_GetBlock(hp_info->fileDesc, i, block));
    data = BF_Block_GetData(block);
    HP_block_info *blinfo = data + hp_info->max * sizeof(Record);
    for (int j = 0; j < blinfo->rec_num; j++)
    {
      Record *rec = data;
      if (value == rec[j].id)
      {
        printRecord(rec[j]);
        counter = i;
      }
    }
    CALL_BF(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return counter;
}
