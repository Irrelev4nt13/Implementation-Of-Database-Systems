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

# Heap File

## General Structure

The structure of the file implemented with `hp_file.c` is similar to the structure of a heap. Given the instructions of the assignment, I used a heap logic to store data/records into the file `data.db`. Each record has a unique id, therefore there is no duplicate record check when inserting records to the file (We ere also encouraged from our instructors not to check for duplicates). <br />
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

## Admissions

The admissions made for this section are the following:

- The first block of the file contains strictly only information about the file and its data blocks. We do not store data/records to it
- Any other block besides the first one stores records starting from the beginning of the block and the local information at the end of the block
- The first byte of each file contains a character which represents the type of the file. The characters 'p' corresponds to a heap file

# Hash Table File

## General Structure

## Technical Details

## Admissions

# Secondary Hash Table File

## General Structure

## Technical Details

Function `int HP_GetAllEntries()` has to find all the record which has as record.name the name given to its arguments. To implement that function we first find which bucket corresponds to the name given. We also define an array "visited" which will help us find all the records we search while we are not traversing a block of the primary index more than once. Every array element corresponds to a block of the primary index and the value is either 0 or 1. If the value is 0 that means that we have not visited this block yet,but if it is 1 that means that we have visited it before and already found what we are looking for, so we dont traverse this block again. The size of the array visited covers the possibility that all the blocks of the primary index has records with field name same as the one we are looking for. After we initialize the array with 0 we then start a while loop which visits all the blocks of the bucket. This way we will get all the block ids that have the name we are looking for. For every block of the bucket we extract the block id which contains the name we are searching. Then we get the data of that block and begin to traverse all of its elements so that we find all the records with the field name "name" and print them out. When we are done searching on this block, we update the corresponding element in the visited array and unpin the block as we no longer need it. Finally we return the number of blocks of the primary index which we searched to find all the records, otherwise we return -1 <br/>

## Admissions

# Hash Statistics

## General Structure

## Technical Details

## Admissions

## Additional functionalities
