#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bf.h"
#include "../include/hp_file.h"

#define RECORDS_NUM 100 // you can change it if you want
#define FILE_NAME "data.db"

int main()
{
  CALL_BF(BF_Init(LRU));

  CALL_BF(HP_CreateFile(FILE_NAME));
  HP_info *info = HP_OpenFile(FILE_NAME);
  Record record;
  srand(time(NULL));
  int r;
  for (int id = 0; id < RECORDS_NUM; ++id)
  {
    record = randomRecord();
    HP_InsertEntry(info, record);
  }
  // printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("%d Blocks traversed till we found all the Records with id %d (If -1 is printed, we didnt find it)\n", HP_GetAllEntries(info, id), id);

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("%d Blocks traversed till we found all the Records with id %d (If -1 is printed, we didnt find it)\n", HP_GetAllEntries(info, id), id);

  HP_CloseFile(info);
  BF_Close();
}