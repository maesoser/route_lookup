#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/********************************************************************
 * Constant definitions
 ********************************************************************/
#define MTABLE_ENTRIES_LENGTH 16777216  // 2^24 entries

/********************************************************************
 * 
 * Reads the Ip's file, and calls interface_lookup() method to get the output interfaces
 * 
 ********************************************************************/
void compute_routes();

/********************************************************************
 *
 *  Generate the TBL24 (called mtable) and TBLlong (called stable) tables
 *
 ********************************************************************/
void initializeFIB();

/********************************************************************
 * 
 * Looks for an IP inside the main and secundary tables.
 * 
 ********************************************************************/
void interface_lookup(uint32_t *IP_lookup,short int *ntables,unsigned short *interface);
