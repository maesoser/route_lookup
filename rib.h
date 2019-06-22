#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/********************************************************************
 * Constant definitions
 ********************************************************************/
#define MTABLE_ENTRIES_LENGTH 16777216  // 2^24 entries

typedef struct RIB {
 short *main_table;		// Main Table
 short *aux_table;		// Second Table
 unsigned short extended_pools;  // Number of networks using the extended table.
 long ip_index;
} RIB_t ;

void RIB_init(RIB_t* self);
void RIB_addRoute(RIB_t* rib, uint32_t *ipv4_addr, int *mask, int *out_iface);
void RIB_lookup(RIB_t* rib, uint32_t *IP_lookup, short int *ntables, unsigned short *iface);
void RIB_destroy(RIB_t* rib);
