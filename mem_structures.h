// This header file is used to implement the data structures
// to be used with the MMU.

#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 512
#define CACHE_SIZE 32

// Translation Lookaside Buffer
// Set Range: (0,15) - 16 sets
// Max Entries: 16 entries
typedef struct Tlb {
	int tag;
	int ppn;
	int valid;
} Tlb;

// Page Table
// Size: 2^9 = 512 entries
// VPN range: (0,511)
typedef struct PageTable {
	int vpn;
	int ppn;
	int valid;
} PageTable;

// Cache (Two-way)
// Size: 32 * 2 = 64
// Range: (0,31)
typedef struct Cache {
	int tag1;
	int tag2;
	int valid1;
	int valid2;
	int data1;
	int data2;
} Cache;

// function used to initialize TLB
struct Tlb* initTlb();
// function used to initialize Page Table
struct PageTable* initPageTable();
// function used to initialize Cache
struct Cache* initCache();
