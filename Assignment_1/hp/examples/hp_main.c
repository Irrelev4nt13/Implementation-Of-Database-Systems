#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bf.h"
#include "../include/hp_file.h"

#define RECORDS_NUM 11 // you can change it if you want
#define FILE_NAME "data.db"

int main()
{
  CALL_BF(BF_Init(LRU));

  CALL_BF(HP_CreateFile(FILE_NAME));
  HP_info *info = HP_OpenFile(FILE_NAME);
  Record record;
  srand(12569874);
  int r;
  // printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id)
  {
    record = randomRecord();
    HP_InsertEntry(info, record);
    if (id == 4)
      printf("%ld\n", sizeof(record));
  }
  // printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d", id);
  printf("%d\n", HP_GetAllEntries(info, 4));
  // HP_GetAllEntries(info, 2);

  HP_CloseFile(info);
  free(info);
  BF_Close();
}