# Assignment 1 & 2 - Κ18 - Implementation of Database Systems Winter Semester 2022 - 2023

## Contributing

Kanellakis Konstantinos - 1115202000064 <br />
Chatzispyrou Michail - 1115202000212

# Compile & Run

## For compilation

- `make bf` to compile the necessary files for the given library `BF`
- `make hp` to compile the necessary files for heap file
- `make ht` to compile the necessary files for hash file
- `make sht` to compile the necessary files for secondary index file
- `make stats` to compile the necessary files for HashStatistics

## For execution

- `make execute_hp` to execute the heap test
- `make execute_hp` to execute the hash test
- `make execute_sht` to execute the secondary index test
- `make execute_stats` to execute the statistics test
# Admissions

The admissions made for this section are the following:

- The first block of each file contains strictly only information about the file and its data blocks also known as metadata. We do not store data/records to it
- Any other block besides the first one stores records starting from the beginning of the block and the local information at the end of the block
- The first byte of each file contains a character which represents the type of the file.
    - `p` corresponds to a heap file
    - `t` corresponds to a hash file
    - `s` corresponds to a secondary heap file
# Heap File

## General Structure

The structure of the file implemented with `hp_file.c` is similar to the structure of a heap. Given the instructions of the assignment, I used a heap logic to store data/records into the file `data.db`. Each record has a unique id, therefore there is no duplicate record check when inserting records to the file (We were also encouraged from our instructors not to check for duplicates). <br />
The general structure of the file is the following:<br />

- First block of the file contains only information about the file and the rest of the blocks
- Any other block except the first one contains records and information about the block

## Technical Details

As it is shown in the `hp_file.h` there are 2 defined structures to help us store the information we need in the file. <br />
The first one `HP_info` stores:

- The type of the file we are working on (It is set to 'p' for a heap file). It is also stored at the very first byte of the file as type `char` so that we can identify its type
- The file Descriptor of the file given by the function `BF_OpenFile()`
- The id of the last block we inserted
- The maximum number of records a block can take

The second one `HP_block_info` stores:

- The number of records a block has currently

The definitions of the functions in the file `hp_file.h` were given by the instructors for us to implement, using the already made functions given in the header file `bf.h`. <br/>
As the code suggests for each of the functions we implemented every pointer of type `BF_Block` which is initialized,it is also being unpinned(setted as dirty if we have made changes to it) and destroyed when is no longer needed. <br/>
<br/>
Function `int *HP_CreateFile()`simply creates the file with the given name **filename** and then opens it, so it can store the essential data to the first block of this file. It is stored the file type in the very first byte of the file,initalized the last block id that we insert with 0 and the file Decsriptor. It is also being calculated and stored the maximum number of records we can store to each block depending on the size of each block and the size of structs`HP_block_info`,`Record`. <br/>

Function `HP_info *HP_OpenFile()` simply opens the file with the given name filename and checks the first byte of the file to check its type. We do not unpin the block as it is the first block of the file and we will need its information until we close the file. Returning NULL if its not a heap file and a pointer to the first block of the file if everyting is ok. <br/>
Function `int *HP_CloseFile()` simply unpins the first block of the file as we no longer need it and closes the file.<br/>

Function `int HP_GetAllEntries()` simply finds the number of blocks that the file has and traverse every block that contains data (every block except the first one) until it finds the record with the id specified in the arguments. As we get the data of a block from the function `BF_GetBlock()` we search into its records. If we find the record we are looking for we stop searching. If not, we unpin the block as we no longer need it and proceed to search the next block. <br/>
Functions `int HP_InsertEntry()` is responsible for to insert the records the heap file.

- If the number of blocks of the file is greater than 1 there are blocks to store data so we proceed to get the block that we inserted last to try and store to it the new record,just like an insertion to a heap. After we get the data of the block we mentioned, we check if the limit of the records stored to it will be exceeded by the new insertion. If that is the case we create a new block. If there is space for the new record we store it to its data field and increment the counter of records of the specific block
- If the number of blocks of the file is equal to 1 that means that only the block that has the information of the file exists and there are no blocks to store data. If that is the case we create a new block
- The number of blocks can not be negative

Creating a new block to store records is implemented as follows. We allocate a block using `BF_AllocateBlock()` and after that we store the new record to the data field of the block. We also set the counter of records of the block to 1 as there is only 1 record, the one we just inserted. It is also stored the id of this block to the data block of the file as it is the block we last inserted and will be the one we will insert future records till it fills.
# Hash Table File
## General Structure
The structure of the file implemented with ht_file.c is similar to the structure of a hash table. Given the instructions of the assignment, I used hash table to map the buckets with blocks to store data/record into the file `data.db`. Each record has a unique id, therefore there is no duplicate record check when inserting records to the file (We were also encouraged from our instructors not to check for duplicates). This implementation is limited to the number of buckets the `block_0` can store. This is calculated by removing from the block size the size of the structure that holds the metadata of the file but with the hashTable been declared only with one index and divide it by the size of a long integer. In otherwords, 
```c
 typedef struct {
    char type_file;
    int fileDesc;
    long int numBuckets;
    int headerBlock;      
    long int numBlocks;     
    long int hashTable[20];
} HT_info;

(BF_BLOCK_SIZE - sizeof(HT_info))/sizeof(long int) = 60
```
We didn't make the hash file works for any number of buckets as we were not asked to do this.
The general structure of the file is the following:
- First block of the file contains only information about the file and the rest of the blocks
- Any other block except the first one contains records and information about the block
## Technical Details
As it is shown in the `ht_file.h` there are 2 defined structures to help us store the information we need in the file.
```c
typedef struct {
    char type_file;         // Store a character accordingly to the file's type, t-> hash, p->heap, s->sht
    int fileDesc;           // file descriptor
    long int numBuckets;    // the number of buckets in the hash table
    int headerBlock;        // the id of the header block
    long int numBlocks;     // the total number of blocks used by hash table
    long int hashTable[20]; // the hash table
} HT_info;

typedef struct {
    long int numRecords; // The total number of records in this block
    long int maxRecords; // The maximum number of records that this block can have
    int nextBF_Block;    // A "pointer" to the next block in case of overflow
} HT_block_info;
``` 
When it comes to the definitions of the functions in the file ht_file.h were given by the instructors for us to implement, using the already made functions given in the header file bf.h.
As the code suggests for each of the functions we implemented every pointer of type BF_Block which is initialized,it is also being unpinned(setted as dirty if we have made changes to it) and destroyed when is no longer needed.
- ```c
    int HT_CreateFile(char *fileName, int buckets)
    ```
The above function creates the hash file and opens it in order to store the metadata of the file. The meta data, consists of the file descriptor, the name of the file, the number of buckets that were given as argument and the header block id. Also in the beginning of the file it stores the type file which is `t` since we are creating a hash file. Last but not least, it initializes the hash table but it doesn't make any allocations yet, they will be made only when the corresponding bucket is asked.
- ```c
    HT_info *HT_OpenFile(char *fileName)
    ```
The function opens the file and checks if the type file is the correct one, then it proceeds by returning a pointer to the metadata of the file which is `HT_info` as we analyzed above.
- ```c
    int HT_CloseFile(HT_info *header_info)
    ```
Firstly, the function get the header block which is where the data of the file are stored and unpin it because we are not gonna need it anymore. Then it destroys the block and close the file throught the file descriptor.
- ```c 
    int HT_InsertEntry(HT_info *header_info, Record record)
    ```
When a record comes we pass its id through the hash function to get the bucket that we are gonna use to store the record. First we check if the asked bucket is mapped with any block, if not then the function creates a block and map it to the corresponding bucket. If the bucket is linked with any bucket it simple get the block. After that it checks if the current block has space to store the record. If the current number of record inside the block does not exceeds the maximum number of records we store the record in the block and unpin the block because we wont need it anymore. Otherwise the function unpins the previous overflowed block and creates a new one but this time the new blocks "points" to the previously overflowed one. As previously mentioned the record is copied but now we are getting the header block to change the meta data of the file because we made a new block which means the total number of blocks that is used by the buckets is changed and also we need to declare that the current bucket is mapped to a new block.
- ```c 
    int HT_GetAllEntries(HT_info *header_info, int value)
    ```
In the beginning we are passing the given value through the hash function to get the bucket that the record is stored in. Then we proceed by initialize a counter which counts the number of block that was used until we find the records with the given value. Then a while loop opens, we keep going backwards block by block. For every block we search all of its record if we find a record with the same id we print it otherwise we then move to the next one. In the mean time every time before we move to the block we unpin the block that was searched because we won't need it anymore.
## Aditional Functions
Also, we implemented one additional helper function:
```c 
int hash(const char, const int);
```
We preferred it to get as argument a string in order to be used in secondary hash file.

# Secondary Hash Table File

## General Structure
The secondary hash table file hash the same draw back as the hash file, it can work for limited number of buckets which can me calculated as before.
## Technical Details
As it is shown in the `ht_file.h` there are 2 defined structures to help us store the information we need in the file.
```c
typedef struct {
    char type_file;         // Store a character accordingly to the file's type, t-> hash, p->heap, s->sht
    int fileDesc;           // file descriptor
    long int numBuckets;    // the number of buckets in the hash table
    int headerBlock;        // the id of the header block
    long int numBlocks;     // the total number of blocks used by hash table
    long int hashTable[20]; // the hash table
    char *filename;  // the file name of the primary index
    char *sfilename; // the file name of secondary index
} SHT_info;

typedef struct {
    long int numRecords; // The total number of records in this block
    long int maxRecords; // The maximum number of records that this block can have
    int nextBF_Block;    // A "pointer" to the next block in case of overflow
} SHT_block_info;

``` 
When it comes to the definitions of the functions in the file ht_file.h were given by the instructors for us to implement, using the already made functions given in the header file bf.h.
As the code suggests for each of the functions we implemented every pointer of type BF_Block which is initialized,it is also being unpinned(setted as dirty if we have made changes to it) and destroyed when is no longer needed.
- ```c
    int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char *fileName)
    ```
The above function creates the hash file and opens it in order to store the metadata of the file. The meta data, consists of the file descriptor the name of the secondary hash file and as the primary one. Moreover, it stores the number of buckets that were given as argument and the header block id. Also in the beginning of the file it stores the type file which is `t` since we are creating a hash file. Last but not least, it initializes the hash table but it doesn't make any allocations yet, they will be made only when the corresponding bucket is asked.
- ```c
    SHT_info *SHT_OpenSecondaryIndex(char *sfileName)
    ```
The function opens the file and checks if the type file is the correct one, then it proceeds by returning a pointer to the metadata of the file which is `HT_info` as we analyzed above.
- ```c
    int SHT_CloseSecondaryIndex(SHT_info *header_info)
    ```
Firstly, the function get the header block which is where the data of the file are stored and unpin it because we are not gonna need it anymore. Then it destroys the block and close the file throught the file descriptor.
- ```c 
    int SHT_SecondaryInsertEntry(SHT_info *header_info, Record record, int block_id)
    ```
When a record comes this time we pass its name through the hash function to get the bucket that we are gonna use to store the record. First we check if the asked bucket is mapped with any block, if not then the function creates a block and map it to the corresponding bucket. If the bucket is linked with any bucket it simple get the block. After that it checks if the current block has space to store the record. If the current number of record inside the block does not exceeds the maximum number of records we store the record in the block and unpin the block because we wont need it anymore. Otherwise the function unpins the previous overflowed block and creates a new one but this time the new blocks "points" to the previously overflowed one. As previously mentioned the record is copied but now we are getting the header block to change the meta data of the file because we made a new block which means the total number of blocks that is used by the buckets is changed and also we need to declare that the current bucket is mapped to a new block.
- ```c 
    int SHT_SecondaryGetAllEntries(HT_info *ht_info, SHT_info *sht_info, char *name) 
    ```

The above function has to find all the record which has as record.name the name given to its arguments. To implement that function we first find which bucket corresponds to the name given. We also define an array "visited" which will help us find all the records we search while we are not traversing a block of the primary index more than once. Every array element corresponds to a block of the primary index and the value is either 0 or 1. If the value is 0 that means that we have not visited this block yet,but if it is 1 that means that we have visited it before and already found what we are looking for, so we dont traverse this block again. The size of the array visited covers the possibility that all the blocks of the primary index has records with field name same as the one we are looking for. After we initialize the array with 0 we then start a while loop which visits all the blocks of the bucket. This way we will get all the block ids that have the name we are looking for. For every block of the bucket we extract the block id which contains the name we are searching. Then we get the data of that block and begin to traverse all of its elements so that we find all the records with the field name "name" and print them out. When we are done searching on this block, we update the corresponding element in the visited array and unpin the block as we no longer need it. Finally we return the number of blocks of the primary index which we searched to find all the records, otherwise we return -1 <br/>

# Hash Statistics

## General Structure

## Technical Details

## Additional functionalities
