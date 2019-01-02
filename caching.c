#include <stdio.h>
#include <stdlib.h>
#include "memory_system.h"
#include "mem_structures.h"
/* Feel free to implement additional header files as needed */

struct Tlb* tlb;
struct PageTable* pageTable;
struct Cache* cache;

void
initialize() {
/* if there is any initialization you would like to have, do it here */
/*  This is called for you in the main program */
	tlb = initTlb();
	pageTable = initPageTable();
	cache = initCache();
}

/* You will implement the two functions below:
 *     * you may add as many additional functions as you like
 *     * you may add header files for your data structures
 *     * you MUST call the relevant log_entry() functions (described below)
 *          or you will not receive credit for all of your hard work!
 */

int
get_physical_address(int virt_address) {
/*
   Convert the incoming virtual address to a physical address. 
     * if virt_address too large, 
          log_entry(ILLEGALVIRTUAL,virt_address); 
          return -1
     * if PPN is in the TLB, 
	  compute PA 
          log_entry(ADDRESS_FROM_TLB,PA);
          return PA
     * else use the page table function to get the PPN:
          * if VPN is in the Page Table
	          compute PA 
                  add the VPN and PPN in the TLB
	          log_entry(ADDRESS_FROM_PAGETABLE,PA);
	          return PA
	  * else load the frame into the page table
	          PPN = load_frame(VPN) // use this provided library function
                  add the VPN and PPN in to the page table
                  add the VPN and PPN in to the TLB
 		  compute PA
		  log_entry(ADDRESS_FROM_PAGE_FAULT_HANDLER,PA);
 		  return PA
*/
	int PA;
	int VA = virt_address;
	struct Tlb* currentTLB = NULL;
	int pageTableIndex = 0;
	struct PageTable* currentPT = NULL;
	int vpn = (VA >> 9) & 0x1ff;
	int vpo = VA & 0x1ff;
	int vpnTag = (VA >> 13) & 0x1f;
	int vpnIndex = (VA >> 9) & 0xf;
	int ppn = 0;
	// checking if VA is too large
	//if((VA >> 13) >= (2 << 9) - 1) {
	if(VA >= (1 << 18) || VA < 0) {
		log_entry(ILLEGALVIRTUAL, VA);
		return -1;
	}
	int tlbIndex = 0;
	// checking index of TLB by vpnIndex
	tlbIndex = sizeof(struct Tlb) * vpnIndex;
	currentTLB = (tlb + tlbIndex);
	// checking if the tlbIndex found is valid and if the vpn tag
	// matches the tag of the tlb entry
	if(currentTLB->valid == 1 && currentTLB->tag == vpnTag) {
		PA = ((currentTLB->ppn) << 9) | vpo;
		log_entry(ADDRESS_FROM_TLB, PA);
		return PA;
	}
	else {
		// checking page table to see if vpnTag is valid
		pageTableIndex = sizeof(struct PageTable) * vpn;
		currentPT = (pageTable + pageTableIndex);
		// getting physical address from page table index and formatting with vpo
		PA = ((currentPT->ppn) << 9) | vpo;
		ppn = (PA >> 9) & 0x7ff;
		// checking if the given vpn is a valid entry in the page table
		if(currentPT->valid == 1) {
			// updating page table
			currentPT = (pageTable + pageTableIndex);
			currentPT->valid = 1;
			currentPT->ppn = ppn;
			currentPT->vpn = vpn;
			// updating TLB with most recent access
			currentTLB->valid = 1;
			currentTLB->ppn = ppn;
			currentTLB->tag = vpnTag;
			log_entry(ADDRESS_FROM_PAGETABLE, PA);
			return PA;
		}
		// entry not valid
		else {
			// no entry in tlb or page table so a new frame is loaded
			ppn = load_frame(vpn);
			PA = (ppn << 9) | vpo;
			// updating page table
			currentPT->vpn = vpn;
			currentPT->ppn = ppn;
			currentPT->valid = 1;
			// updating tlb
			currentTLB->valid = 1;
			currentTLB->ppn = ppn;
			currentTLB->tag = vpnTag;
			// logging
			log_entry(ADDRESS_FROM_PAGE_FAULT_HANDLER, PA);
		}
	}
	return PA;
}

char
get_byte(int phys_address) {
/*
   Use the incoming physical address to find the relevant byte. 
     * if data is in the cache, use the offset (last 2 bits of address)
          to compute the byte to be returned data
          log_entry(DATA_FROM_CACHE,byte);
          return byte 
     * else use the function get_long_word(phys_address) to get the 
          4 bytes of data where the relevant byte will be at the
          given offset (last 2 bits of address)
          log_entry(DATA_FROM_MEMORY,byte);
          return byte

NOTE: if the incoming physical address is too large, there is an
error in the way you are computing it...
*/
	char byte;
	int mem_access = 0;
	int PA = phys_address;
	int offset = PA & 0x3;
	int index = (PA >> 2) & 0x1f;
	int cacheTag = (PA >> 7) & 0x1fff;
	int cacheIndex = 0;
	cacheIndex = sizeof(struct Cache) * index;
	// storing the current cache index based on the index value
	// returieved from the physical address
	struct Cache* current = (cache + cacheIndex);
	// checking cache for address
	if(current->tag1 == cacheTag) {
		if(current->valid1 == 1) {
			byte = ((current->data1) >> (8 * offset)) & 0xff;
			log_entry(DATA_FROM_CACHE, byte);
			return byte;
		}
	}
	if(current->tag2 == cacheTag) {
		if(current->valid2 == 1) {
			byte = ((current->data2) >> (8 * offset)) & 0xff;
			log_entry(DATA_FROM_CACHE, byte);
			return byte;
		}
	}
	// retrieving data from memory since there were no cache hits
	mem_access = get_word(PA);
	int* ptr = &mem_access;
	//TEST
	byte = (mem_access >> (8 * offset)) & 0xff;
	//byte = mem_access;
	log_entry(DATA_FROM_MEMORY, byte);
	// updating cache
	// first swapping first entry with second entry since it is newer
	current->tag2 = current->tag1;
	current->valid2 = current->valid1;
	current->data2 = current->data1;
	// now updating cache entry 1 at given index with new data from memory
	current->tag1 = cacheTag;
	current->valid1 = 1;
	current->data1 = mem_access;

	return byte;
}

// HELPER FUNCTIONS
// function used to initialize TLB
struct Tlb* initTlb() {
	int index = 0;
	struct Tlb* tlb = (struct Tlb*)malloc(sizeof(struct Tlb) * TLB_SIZE);
	struct Tlb* current = tlb;
	for(int i = 0; i < TLB_SIZE; i++) {
		index = sizeof(struct Tlb) * i;
		current = (tlb + index);
		current->tag = 0;
		current->ppn = 0;
		current->valid = 0;
	}
	return tlb;
}

// function used to initialize page table
struct PageTable* initPageTable() {
	int index = 0;
	struct PageTable* pageTable = (struct PageTable*)malloc(sizeof(struct PageTable) * PAGE_TABLE_SIZE);
	struct PageTable* current = pageTable;
	for(int i = 0; i < PAGE_TABLE_SIZE; i++) {
		index = sizeof(struct PageTable) * i;
		current = (pageTable + index);
		current->vpn = 0;
		current->ppn = 0;
		current->valid = 0;
	}
	return pageTable;
}

// function used to initialize Cache
struct Cache* initCache() {
	int index = 0;
	struct Cache* cache = (struct Cache*)malloc(sizeof(struct Cache) * CACHE_SIZE);
	struct Cache* current = cache;
	for(int i = 0; i < CACHE_SIZE; i++) {
		index = sizeof(struct Cache) * i;
		current = (cache + index);
		current->tag1 = 0;
		current->tag2 = 0;
		current->valid1 = 0;
		current->valid2 = 0;
		current->data1 = 0;
		current->data2 = 0;
	}
	return cache;
}

