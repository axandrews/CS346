/*
Basics of program needed to complete assignment 8
This example processes the first virtual address stored in addresses.txt
The output should be identical to the first line of correct.txt
usage: ./a.out BACKING_STORE.bin addresses.txt
BACKING_STORE.bin and addresses.txt must bin the same directory as the executable
*/

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TLB_SIZE 16
#define PAGES 256
#define PAGE_MASK 255
#define PAGE_SIZE 256
#define OFFSET_BITS 8
#define OFFSET_MASK 255
#define MEMORY_SIZE PAGES * PAGE_SIZE 


/*
********Important Types, Data Structures and Variables************************ 
Structs
tlb: struct representing an entry in translation lookaside buffer 
virtAddress: struct representing the page offset within the page table and displacement within page

Storage
backing: pointer to memory mapped secondary storage
main_memory: array of signed chars simulating RAM

Tables
tlBuf: array of type tlb representing the translation lookaside buffer
tlbindex: index into tlBuf
pagetable: array of ints simulating the page table

Counters
numPageRefs: number of page table references (references = tries or hits? where is this used?)
numPageFaults: counts number of page faults
numTLBRefs: number of TLB tries (so up to 16000 for 1000 * 16? )
numTLBHits: number of TLB references that resulted in a hit 

Addresses 
logical_address: address read from disk, simulating an address reference 
virtualAddress: variable of type virtAddress. Parse of logical_address into page number and offset
physical_page: page frame number
physical_address: bytes from the 0th byte of RAM, i.e., the actual physical address 
logical_page: page table number
offset: displacement within page table/frame 

Output
Virtual address: logical_address, in Addresses, above
Physical address: physical_address in Addresses, above 
value: value stored in main_memory at physical_page displacement plus offset
*/

struct tlbentry 
   {
    unsigned char logical;
    unsigned char physical;
    int last_referenced;
   };
typedef struct tlbentry tlb;  

// I understand we could make this from the logical address
// but why would we it seems like an extra step we don't store these
struct virtAddress 
{
    int logical_page;  //page/frame number 
    int offset;
};
typedef struct virtAddress virtualAddress;  


int main(int argc, char *argv[])
{ 
    int pagetable[PAGES];
    int in_ram[PAGES] = {0}; // initialize to all 0s, represents which pages are in ram 
    tlb tlBuff[TLB_SIZE];
    int tlbindex = 0;
    signed char main_memory[MEMORY_SIZE];
    signed char *backing;
    int logical_address;
    int offset;
    int logical_page;
    int physical_page;
    int physical_address;
    signed char value;
    
    int numAddresses = 0;
    
    int numPageFaults = 0;
    int numPageRefs = 0;
    int numTLBRefs = 0;
    int numTLBHits = 0;
    int found;
    
    if (argc != 3)
    {
        fprintf(stderr, "usage: ./a.out BACKING_STORE.bin addresses.txt");
        return 1;
    }
    
    //open simulation of secondary storage     
    const char *backing_filename = argv[1]; //BACKING_STORE.bin 
    int backing_fd = open(backing_filename, O_RDONLY);
    //after the next instruction the secondary storage file can be viewed as an array
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 

    //open simulation of address bus and read the first line 
    FILE *ifp = fopen(argv[2],"r"); //addresses.txt 
    
    
    // When you write the program, the physical page will increment by 1 for each copy
    // from simulated backing store to main memory (page fault - disk to RAM)
    physical_page = 0; 
    while(fscanf(ifp,"%d", &logical_address) != EOF)
    {
         found = 0;
         //extract low order 8 bits from the logical_address. This is the offset
        offset = logical_address & OFFSET_MASK;
        //extract bits 8 through 15. This is the page number gotten by shifting right 8 bits 
        logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;

        // loop through TLB and look for the logical address
        numAddresses++;
        // parallel search??? how
        int i;
        int num_items;
        if (tlbindex < TLB_SIZE)
            num_items = tlbindex;
        else
            num_items = TLB_SIZE;
        for (i = 0; i < num_items && !found; i++) // loop through however many items are in tlb
        {
            numTLBRefs++;
            if ((int)tlBuff[i].logical == logical_page) // this is a char. compared to an int.  
            {
                found = 1;
                int frame = tlBuff[i].physical; 
                physical_address = (frame << OFFSET_BITS) | offset; 
                //extract the value store at offset bytes within the page
                value = main_memory[offset + frame * PAGE_SIZE]; 
                tlBuff[i].last_referenced = numAddresses;  // update when it was last referenced to now
            }
        }
        
        // if not in TLB, go to reference in page table and look for the logical address
        // how do we tell if it is there though? Currently an extra array.
        if (!found && in_ram[logical_page]) // not in tlb but in ram
        {
            int frame = pagetable[logical_page]; 
            physical_address = (frame << OFFSET_BITS) | offset; // Shift left 8 bits then combo frame and offset
            //extract the value store at offset bytes within the page
            value = main_memory[offset + frame * PAGE_SIZE]; 
            
            /*
            // update tlb 
            if (tlbindex < TLB_SIZE)  // if the TLB is not yet full. not needed in ram section.
            {
                tlb new_tlb_entry;
                new_tlb_entry.logical = logical_page;
                new_tlb_entry.physical = physical_page;
                new_tlb_entry.last_referenced = numAddresses; 
                tlBuff[tlbindex] = new_tlb_entry;
                tlbindex++;
            } */
            // otherwise we need to find a page to replace
            int min = numAddresses;  // a starting point higher than things in tlb
            int min_location = -1;
            int i;
            for (i = 0; i < TLB_SIZE; i++) 
            {
                if (tlBuff[i].last_referenced < numAddresses)
                {
                    min = tlBuff[i].last_referenced;
                    min_location = i;
                }
            }
            if (min_location == -1) 
                fprintf(stderr, "Fatal error during TLB replacement"); 
            // add the new page, replacing the least recent
            tlb new_tlb_entry;
            new_tlb_entry.logical = logical_page;
            new_tlb_entry.physical = physical_page;
            new_tlb_entry.last_referenced = numAddresses; 
            tlBuff[min_location] = new_tlb_entry;
        } 
        else // otherwise page fault and get from disk 
        {
            numPageFaults++;
            // if the logical address is in neither the TLB or the page table
            //copy from secondary storage to simulated RAM. The address on secondary storage
            //as an offset into the memory map, backing, computing by multiplying the logical
            //page number by 256 and adding the offset 
            memcpy(main_memory + physical_page * PAGE_SIZE, 
                   backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            pagetable[logical_page] = physical_page;
            in_ram[logical_page] = 1;  // so we can check that it is in RAM
            
            //Shift the physical page left 8 bits and or with the offset
            physical_address = (physical_page << OFFSET_BITS) | offset;
            //extract the value store at offset bytes within the page
            value = main_memory[physical_page * PAGE_SIZE + offset];
            physical_page++;
            
            // update tlb
            if (tlbindex < TLB_SIZE)  // if the TLB is not yet full
            {
                tlb new_tlb_entry;
                new_tlb_entry.logical = logical_page;
                new_tlb_entry.physical = physical_page;
                new_tlb_entry.last_referenced = numAddresses; 
                tlBuff[tlbindex] = new_tlb_entry;
                tlbindex++;
                printf("New entry added to tlb. The index is %d", tlbindex);
            }
            else 
            {  // otherwise we need to find a page to replace
                int min = numAddresses;  // a starting point higher than things in tlb
                int min_location = -1;
                int i;
                printf("replacing a page in the tlb");
                for (i = 0; i < TLB_SIZE; i++) 
                {
                    if (tlBuff[i].last_referenced < numAddresses)
                    {
                        min = tlBuff[i].last_referenced;
                        min_location = i;
                    }
                }
                if (min_location == -1) 
                    fprintf(stderr, "Fatal error during TLB replacement"); 
                // add the new page, replacing the least recent
                tlb new_tlb_entry;
                new_tlb_entry.logical = logical_page;
                new_tlb_entry.physical = physical_page;
                new_tlb_entry.last_referenced = numAddresses; 
                tlBuff[min_location] = new_tlb_entry;
            }
            
        }
        printf("Virtual address: %d Physical address: %d Value: %d\n", 
              logical_address, physical_address, value);  
    }
    
    printf("Number of Translated Addresses = %d\n", numAddresses);
    printf("Page Faults = %d\n", numPageFaults);
    printf("Page Fault Rate = %f\n", ((float)numPageFaults)/numAddresses); 
    printf("TLB Hits = %d\n", numTLBHits);
    printf("TLB Hit Rate = %f\n", ((float)numTLBHits)/numAddresses);
    
    return 0;
}
