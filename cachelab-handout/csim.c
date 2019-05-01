/*
* Name: Xavier Ruiz
* UID: U59232360
* */

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


int nhits, nmisses, nevics; // nbytes


typedef struct {
    int numBytes;
    int tag;
    int valid;
    int dirty;
    int lru;
    
    //char* block; // this is the data for your line.. you actually don't do much
                 // with this as you are not manipulating any actual data here
                 // this is just good to have for future implementation but may be omitted
                 // If you actually wanted to get data from an entry, you would 
                 // index into this array using the block offset and pull the 
                 // requested number of bytes as well
} entry; //an entry is simply a line in the cache

typedef struct {
    entry* entries; // array of entries
    int numEntries;
    int setNum;
} set;

typedef struct {
    set* sets; // array of sets
    int numSets;
} cache;


cache iCache;

void print_usage() {
    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    exit(EXIT_FAILURE); 
}

void print_entry(entry e){
    printf("------------------");
    printf("\ntag:   %d", e.tag);
    printf("\nvalid: %d", e.valid);
    printf("\nlru:   %d", e.lru);
    printf("\ndirty: %d\n", e.dirty);
    printf("------------------\n");

}


char* parse(char* line, char delimeter) {
   if(line[0]==' '){ //if it starts with a space, remove it
        line++;    
    } 
    char * end;
    end = strchr(line,delimeter);
    int length;
    if(delimeter==' '){
        length = end-line;
        //printf("DELIMETER:[SPACE]
    } else if (delimeter==','){
        length = end-strchr(line,' ');
        //printf("DELIMETER:,\n");
    } else if (delimeter=='\n'){
        length = end-strchr(line,',');
        //printf("DELIMETER:\\n\n");
    } else {
        printf("Error: incorrect delimeter");
        exit(EXIT_FAILURE);
    }
    //printf("LENGTH:%d\n", length);
    char* newString=malloc(sizeof(char)*(length+1)); // plus one for termination char
    memcpy(newString, end-length, length); // rewind to beginning of respective section
    while(newString[0]==' '||newString[0]==',') newString++; // remove extra characters
    newString[length]='\0'; // termination character
    return newString; 
}

unsigned int createMask(unsigned n)
{
   unsigned r = 0;
   for (unsigned i=0; i<=n; i++)
       r |= 1 << i;
   return r>>1;
}

int findEntry(set currentSet, int tag, int *smallest_lru){
    *smallest_lru = currentSet.entries[0].lru;
    for(int i=0;i<currentSet.numEntries; i++){
        if(currentSet.entries[i].tag==tag) return 1;
        if(currentSet.entries[i].lru<*smallest_lru) *smallest_lru = currentSet.entries[i].lru;
    }
    return 0;
}


int main(int argc, char **argv)
{
    int setBits=0, lines=0, blockBits=0;
    char* trace;
    if(argc<5){
        print_usage();
    }
    int option=0;
    int sFlag=0, eFlag=0, bFlag=0, tFlag=0, hFlag=0, vFlag=0; // by keeping track of when flags are set, you cannot set flags twice
    while((option=getopt(argc,argv,"s:E:b:t:hv")) != -1){ // the string part says that s,E,b,t must be followed by a parameter
        switch(option){
            case 's':
                if(sFlag>0) print_usage();
                //printf("s argument, number of index bits: %d\n", (int)atoi(optarg));
                setBits = (int)atoi(optarg); 
                sFlag++; 
                break;                
            case 'E':
                if(eFlag>0) print_usage();
                // printf("E argument, number of lines per set: %d\n", (int)atoi(optarg));
                lines = (int)atoi(optarg);
                eFlag++;
                break;
            case 'b':
                if(bFlag>0) print_usage();
                //printf("b argument, number of block bits: %d\n", (int)atoi(optarg));
                blockBits = (int)atoi(optarg);
                bFlag++;
                break;                
            case 't':
                if(tFlag>0) print_usage(); 
                //printf("t argument, tracefile name: %s\n", optarg);
                trace = optarg;
                tFlag++;
                break;
            case 'h':
                if (hFlag>0) print_usage();
                print_usage(); 
                break;
            case 'v':
                if(vFlag>0) print_usage();
                printf("THE VERBOSE FLAG HAS BEEN SET\n");
                vFlag++;
                break;
            default:
                print_usage();
        }
    }
    if((sFlag==1 && eFlag==1 && bFlag==1 && tFlag==1) == 0) print_usage(); // required arguments not entered
    printf("Number of sets: %d, setBits: %d\n", (1<<setBits), setBits);
    printf("Lines per set: %d\n", lines);
    printf("Block size: %d, blockBits: %d\n", (1<<blockBits), blockBits);
    //printf("file location: %s\n", trace);      

    // CREATE CACHE
    //cache iCache;  // for 'instantiated' cache
    iCache.numSets = (1<<setBits);
    //printf("\niCache.numSets: %d\n", iCache.numSets);
    iCache.sets = malloc(sizeof(set)*iCache.numSets);
    if(iCache.sets==NULL) {printf("Out of memory\n"); exit(EXIT_FAILURE);}
    for(int i=0; i<iCache.numSets; i++){
        set tSet;
        tSet.numEntries = lines;
        tSet.setNum = i;
        tSet.entries = malloc(sizeof(entry)*lines);
        if(tSet.entries==NULL){printf("Out of memory\n"); exit(EXIT_FAILURE);}
        for(int j=0; j<tSet.numEntries; j++){
            entry tEntry;
            tEntry.numBytes = (1<<blockBits);
            tEntry.tag = 0;
            tEntry.valid = 0;
            tEntry.lru = 0;
            tEntry.dirty=0;
            //tEntry.block = malloc(blockBits*sizeof(char)); // initialize the array of charcter pointers which make up the block data
            //if(tEntry.block==NULL) {printf("Out of memory\n"); exit(EXIT_FAILURE);}
            //for(int k=0; k<blockBits; k++){
            //    tEntry.block[k] = '0'; // '0' is arbitrary
            //}
            tSet.entries[j] = tEntry;
        }
        iCache.sets[i] = tSet;
    }
     
      
    // BEGIN FILE READING HERE
    FILE *fp;
    fp = fopen(trace, "r");
    if(fp==NULL) print_usage();
    char lineOfFile[1024]; // max characters is 1024
    while(fgets(lineOfFile,sizeof(lineOfFile),fp)!=0) {
        char *line = lineOfFile;
        char firstArg = *parse(line, ' ');    // what we want to do with the cache
                                              // L=load, S=store, I=do nothing?, M=load and store/modify
        if (firstArg=='I'){ // Do nothing
            continue; // moves onto the next line of the file
        }

        if(!(firstArg=='L'||firstArg=='S'||firstArg=='M')){
            printf("ERROR: invalid first argument\n");
            exit(EXIT_FAILURE);    
        }

        char secondArg = *parse(line, ',');   // physical address, which also contains the
                                              // set index, block offset, and tag
        // char thirdArg = *parse(line, '\n');   // how many bytes to manipulate from the block offset
        // The second arg is the physical address in the cache, we must parse this
        // Let us first get the address as a number
        unsigned int physicalAddr =  strtol(&secondArg, NULL, 16);      // 16 because all addresses are passed in as hex
        // The physical address contains the blockOffset, setIndex, and cacheTag
        //int blockOffset = physicalAddr&createMask(blockBits);         // the lowest bits
        int setIndex = (physicalAddr>>blockBits)&createMask(setBits); // The next lowest bits are your set index
        int cacheTag = physicalAddr&~createMask(blockBits+setBits);   // the rest of the bits are the tag
        //printf("\naddress: %d\n",   physicalAddr); 
        printf("\nsetIndex: %d\n",        setIndex); 
        printf("cacheTag: %d\n",        cacheTag);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        //printf("blockOffset: %d\n",  blockOffset);  // the block offset is unused in this simulation
         
        int entryCounter = 0;
        int entryFound = 0;
        int smallest_lru = iCache.sets[setIndex].entries[0].lru; //initialize to first entry lru
        // go through entries and try and find matching tag    
        // if not found, it keeps track of lowest lru for later
        while(entryCounter++<iCache.sets[setIndex].numEntries){    
            //printf("This is entry: %d, with tag: %d\n", entryCounter-1, currentEntry.tag);
            if(iCache.sets[setIndex].entries[entryCounter-1].tag==cacheTag) {
                entryFound = 1;
            }
            if(iCache.sets[setIndex].entries[entryCounter-1].lru<smallest_lru) smallest_lru=iCache.sets[setIndex].entries[entryCounter-1].lru; // update the smallest lru as we go along 
        }

        if(entryFound!=1) { // tag not found, find which entry you must place/replace based on lru
            printf("NO MATCHING TAG\n");
            entryCounter = 0;
            // look for the first entry that has the lru num (there can be multiple)
            while(iCache.sets[setIndex].entries[entryCounter].lru==smallest_lru) { entryCounter++; }
            
            print_entry(iCache.sets[setIndex].entries[entryCounter]); 



            if(firstArg=='L'){
                nmisses++;
                iCache.sets[setIndex].entries[entryCounter].lru++; 
                iCache.sets[setIndex].entries[entryCounter].tag=cacheTag;
                printf("newTag: %d\n", iCache.sets[setIndex].entries[entryCounter].tag);
                printf("You want to load/store bits from set %d, line %d\n", setIndex, entryCounter);
                if(iCache.sets[setIndex].entries[entryCounter].valid==1) nevics++; 
                iCache.sets[setIndex].entries[entryCounter].valid=1;
            
            }else if(firstArg=='S'){

            } else if(firstArg=='M') {
                nmisses++;
                if(iCache.sets[setIndex].entries[entryCounter].valid==1) nevics++;
                nhits++;
                iCache.sets[setIndex].entries[entryCounter].lru+=2;
                printf("You want to load&store bits from set %d, line %d\n", setIndex, entryCounter);
                printf("NYI\n");
            } else {
                printf("ERROR: invalid first argument\n");
                exit(EXIT_FAILURE);
            }
        } else { // matching tag has been found!
            if(firstArg=='L'){
                printf("operation: load\n");
            } else if(firstArg=='S') {
                printf("operation: store\n");
            } else {
                printf("operation: modify\n");
            }
            print_entry(iCache.sets[setIndex].entries[entryCounter]); 
            printf("MATCHING TAG FOUND\n");
            if(iCache.sets[setIndex].entries[entryCounter].valid==0) { // oops, not valid
                nmisses++;
                
            } else { // valid data!!
                // We also want to check that we don't have to evict data
                // If we are simply loading data, we do not have to evict
                if(iCache.sets[setIndex].entries[entryCounter].dirty==1&&firstArg=='S'){
                    nevics++;
                }
                iCache.sets[setIndex].entries[entryCounter].lru++;
                iCache.sets[setIndex].entries[entryCounter].valid=1;

            } 
        }      
        
    }

    fclose(fp);

    printSummary(nhits,nmisses,nevics); //printSummary(hit_count, miss_count, eviction_count);
    return EXIT_SUCCESS;
}

