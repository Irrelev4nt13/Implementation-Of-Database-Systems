#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 1 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

#define CALL_OR_DIE(call)         \
    {                             \
        BF_ErrorCode code = call; \
        if (code != BF_OK)        \
        {                         \
            BF_PrintError(code);  \
            exit(code);           \
        }                         \
    }
int HashStatistics(char *filename /* όνομα του αρχείου που ενδιαφέρει */);

int main(void)
{
    srand(1);
    BF_Init(LRU);
    // Αρχικοποιήσεις
    HT_CreateFile(FILE_NAME, 5);
    SHT_CreateSecondaryIndex(INDEX_NAME, 5, FILE_NAME);
    HT_info *info = HT_OpenFile(FILE_NAME);
    SHT_info *index_info = SHT_OpenSecondaryIndex(INDEX_NAME);

    // Θα ψάξουμε στην συνέχεια το όνομα searchName
    // Record record = randomRecord();
    // char searchName[15];
    // strcpy(searchName, record.name);

    // // Κάνουμε εισαγωγή τυχαίων εγγραφών τόσο στο αρχείο κατακερματισμού τις οποίες προσθέτουμε και στο δευτερεύον ευρετήριο
    // printf("Insert Entries\n");
    // for (int id = 0; id < RECORDS_NUM; ++id)
    // {
    //     record = randomRecord();
    //     int block_id = HT_InsertEntry(info, record);
    //     SHT_SecondaryInsertEntry(index_info, record, block_id);
    // }
    // // Τυπώνουμε όλες τις εγγραφές με όνομα searchName
    // printf("RUN PrintAllEntries for name %s\n", searchName);
    // SHT_SecondaryGetAllEntries(info, index_info, searchName);
    // Κλείνουμε το αρχείο κατακερματισμού και το δευτερεύον ευρετήριο
    SHT_CloseSecondaryIndex(index_info);
    HT_CloseFile(info);
    //
    HashStatistics(FILE_NAME);
    BF_Close();
}
int HashStatistics(char *filename /* όνομα του αρχείου που ενδιαφέρει */)
{
    int fileDesc;
    void *data;
    BF_Block *block;

    BF_Block_Init(&block);

    CALL_OR_DIE(BF_OpenFile(filename, &fileDesc));
    CALL_OR_DIE(BF_GetBlock(fileDesc, 0, block));

    data = BF_Block_GetData(block);
    char *type_file = data;
    printf("%c\n", type_file[0]);
    if (type_file[0] == 't')
    {
        HT_info *info = data;
    }
    else if (type_file[0] == 's')
    {
        SHT_info *info = data;
    }
    else
    {
        printf("Input file not supported for stats\n");
        return -1;
    }
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_CloseFile(fileDesc));
    return 0;
}