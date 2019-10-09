/********************************************************************
 * Routing algorithm based on Two-level hardware multi-bit trie
 *
 * Created by Sergio Maeso Jiménez
 *
 * Universidad Carlos III, Madrid 20 de Febrero de 2015
 *
 ********************************************************************/

#include "rib.h"

// Constructor (without allocation)
void RIB_init(RIB_t* rib) {
   rib->main_table = calloc(MTABLE_ENTRIES_LENGTH,sizeof(short));
   rib->extended_pools = 0;
   rib->ip_index = 0;
}

void RIB_addRoute(RIB_t* rib, uint32_t *ipv4_addr, int *mask, int *out_iface){
    long int n_hosts = 0;	// We calculate the number of hosts affected by the mask
    if(*mask <= 24){
        n_hosts = pow(2,24 - *mask);
	for(rib->ip_index = 0; rib->ip_index < n_hosts; rib->ip_index++)
	{
	    rib->main_table[(*ipv4_addr>>8) + rib->ip_index] = *out_iface;
	}
	}else{
	    n_hosts = pow(2,32 - *mask);
	    if(rib->main_table[*ipv4_addr>>8]>>15 == 0)
	    {
	        // 1. REALLOC MEMORY, we reserve 256 more chunks for the new interfaces
		rib->aux_table = realloc(rib->aux_table, 256*(rib->extended_pools + 1)*2);
		// 2. COPY FROM MTABLE TO STABLE
		for(rib->ip_index = 0; rib->ip_index <= 255; rib->ip_index++)
		{
		    rib->aux_table[rib->extended_pools*256 + rib->ip_index] = rib->main_table[*ipv4_addr>>8];
		}
		// 3. UPDATE MTABLE VALUE WITH THE INDEX OF STABLE
		rib->main_table[*ipv4_addr>>8] = rib->extended_pools | 0b1000000000000000;
		// 4. POPULATE THE STABLE CHUNK WITH THE SPECIFIED NEW ADDRESS
		for(rib->ip_index = (*ipv4_addr & 0xFF); 
		    rib->ip_index < n_hosts + (*ipv4_addr & 0xFF); 
		    rib->ip_index++)
		{
		    rib->aux_table[rib->extended_pools*256 + rib->ip_index] = *out_iface;
		}
		    rib->extended_pools++;
		}
		else{  // If it already exists a chunk for this Ip range inside stable
		    for(rib->ip_index = (*ipv4_addr & 0xFF); 
		        rib->ip_index < n_hosts + (*ipv4_addr & 0xFF); 
		        rib->ip_index++
		    ){
		        rib->aux_table[(rib->main_table[*ipv4_addr>>8] & 0x7FFF)*256 + rib->ip_index] = *out_iface;
		    }
		}
	}
}

/**
 * [Look for an IP address inside the main table and secundary table stored in RAM]
 * Input:
 * 		IP_lookup
 * Output.
 * 		interface
 * 		ntables
 */
void RIB_lookup(RIB_t* rib, uint32_t *IP_lookup, short int *ntables, unsigned short *iface)
{
	*iface = rib->main_table[*IP_lookup>>8];
	if(*iface>>15 == 0)
	{
		*ntables = 1;
		return;
	}
	else
	{
		*ntables = 2;
		*iface = rib->aux_table[(*iface & 0x7FFF)*256 + (*IP_lookup & 0x000000FF)];
		// 0x7fff = 0b0111111111111111 to adquire just the address to the 2nd table
		return;
	}
	return;
}

void RIB_destroy(RIB_t* rib) {
  if (rib) {
		free(rib->main_table);
		free(rib->aux_table);
  }
}
