/********************************************************************
 * Routing algorithm based on Two-level hardware multi-bit trie
 *
 * Created by Sergio Maeso Jiménez
 *
 * Universidad Carlos III, Madrid 20 de Febrero de 2015
 *
 ********************************************************************/

#include "io.h"
#include "my_route_lookup.h"

int error_code;		// In this variable we save the error codes that produces some methods in "io.c"
/** VARIABLES RELATED with the tables **/
short *mtable;		// Main Table
short *stable;		// Second Table
unsigned short extended_IPs;  // Number of networks using the extended table.
/** VARIABLES RELATED WITH THE TABLES INIZIALIZATION **/
long ip_index;
uint32_t *IP_addr;  // We can access each byte of the IP separately http://stackoverflow.com/questions/2747219/how-do-i-split-up-a-long-value-32-bits-into-four-char-variables-8bits-using
int *aux_prefixLength;
int *aux_outInterface;
/*** VARIABLES RELACIONADAS CON LOOKUP */
struct timeval start, end;
/*** VARIABLES RELATED WITH THE PERFORMANCE INFORMATION*/
int *processedPackets;
double *totalTableAccesses;
double *totalPacketProcessingTime;

int main( int argc, char *argv[] )
{
	/* Initializing Variables */
	mtable = calloc(MTABLE_ENTRIES_LENGTH,sizeof(short));
	//stable = malloc(STABLE_ENTRIES_LENGTH*STABLE_ENTRIES_SIZE);
	// We are gonna use dinamically allocated custom tables instead of just one table because the paper approach is designed to be implemented in hardware,
	// and that's more logical in a software oriented implementation.
	processedPackets  = calloc(1,sizeof(int));
	totalTableAccesses  = calloc(1,sizeof(double));
	totalPacketProcessingTime  = calloc(1,sizeof(double));
	error_code = 0;
	extended_IPs = 0;
	if( argc == 3 )
	{
		error_code = initializeIO(argv[1], argv[2]); //Initialize Input
		if(error_code != 0){
			printf("\nERROR: \n\t");
			printIOExplanationError(error_code);
			return -1;
		}
		initializeFIB();	// Load Forwarding table to RAM
		compute_routes();
		printSummary(*processedPackets, (*totalTableAccesses / *processedPackets), (*totalPacketProcessingTime / *processedPackets));
		freeIO();			/* Freeing Resources */
		free(mtable);
		free(stable);
		free(processedPackets);
		free(totalTableAccesses);
		free(totalPacketProcessingTime);

	}
	else
	{
		printf("\nUSE:\n\t./my_route_lookup <routingTable> <inputFile>\n\nWHERE:\n\t<routingTable>\t\tThe path for the file containing the routing table\n\t<inputFile>\t\tThe path for the input file\n\n");
		return -1;
	}
	return 1;
}

 /**
 * [Initialize the Forwarding tables mtable and stable]
 * 	Output:
 * 			mtable
 * 			stable
 */
void initializeFIB()
{
	IP_addr = calloc(1,sizeof(int));
	aux_prefixLength = calloc(1,sizeof(int));
	aux_outInterface = calloc(1,sizeof(int));
	//Now we have the prefix, the ip and the interface
	error_code = readFIBLine(IP_addr, aux_prefixLength, aux_outInterface);
	while(error_code == 0){  //WHILE NOT EOF OR ANOTHER TYPE OF ERROR
		long int number_of_hosts = 0;	// We calculate the number of hosts affected by the mask
		// 2 24 - PREFIJO
		if(*aux_prefixLength <= 24){
			number_of_hosts = pow(2,24 - *aux_prefixLength);
			for(ip_index = 0; ip_index < number_of_hosts; ip_index++) // we run over all the Ip's affected by the mask VERY UNEFFICENT
			{
				mtable[(*IP_addr>>8) + ip_index] = *aux_outInterface;
			}
		}
		else{
			number_of_hosts = pow(2,32 - *aux_prefixLength);
			if(mtable[*IP_addr>>8]>>15 == 0)
			{
				// 1. REALLOC MEMORY, we reserve 256 more chunks for the new interfaces
				stable = realloc(stable, 256*(extended_IPs + 1)*2);
				// 2. COPY FROM MTABLE TO STABLE
				// recorremos todo el rango de IP's del ultimo byte de la IP, copiando lo anterior
				for(ip_index = 0; ip_index <= 255; ip_index++) 
				{
					stable[extended_IPs*256 + ip_index] = mtable[*IP_addr>>8];
				}
				// 3. UPDATE MTABLE VALUE WITH THE INDEX OF STABLE
				// We write the "index" to the address in the stable and the bit 1 in the 16th position (0b1000000000000000)
				mtable[*IP_addr>>8] = extended_IPs | 0x8000;
				// 4. POPULATE THE STABLE CHUNK WITH THE SPECIFIED NEW ADDRESS
				for(ip_index = (*IP_addr & 0xFF); ip_index < number_of_hosts + (*IP_addr & 0xFF); ip_index++)
				{
					stable[extended_IPs*256 + ip_index] = *aux_outInterface;
				}
				extended_IPs++;
			}
			else{  // If it already exists a chunk for this Ip range inside stable
				for(ip_index = (*IP_addr & 0xFF); ip_index < number_of_hosts + (*IP_addr & 0xFF); ip_index++)
				{
					stable[(mtable[*IP_addr>>8] & 0x7FFF)*256 + ip_index] = *aux_outInterface;
				}
			}
		}
	  //Now we get another IP, interface and interface
          error_code = readFIBLine(IP_addr,aux_prefixLength,aux_outInterface);        
	}

			free(IP_addr);
			free(aux_prefixLength);
			free(aux_outInterface);
		}

/**
 * [Look for an IP address inside the main table and secundary table stored in RAM]
 * Input:
 * 		IP_lookup
 * Output.
 * 		interface
 * 		ntables
 */
void interface_lookup(uint32_t *IP_lookup, short int *ntables,unsigned short *interface)
{
	*interface = mtable[*IP_lookup>>8];
	if(*interface>>15 == 0)
	{
		*ntables = 1;
		return;
	}
	else
	{
		*ntables = 2;
		*interface = stable[(*interface & 0x7FFF)*256 + (*IP_lookup & 0x000000FF)];
		// 0x7fff = 0b0111111111111111 to adquire just the address to the 2nd table
		return;
	}
	return;
}

/**
 * [Perform routing process, going through the file and looking for the best Interface for each IP]
 *
 * Output:
 * 		processedPackets
 * 		totalTableAccesses
 * 		totalPacketProcessingTime
 */
void compute_routes()
{
	uint32_t *IP_lookup = calloc(1,sizeof(uint32_t));
	unsigned short *interface = calloc(1,sizeof(unsigned short));
	double *searching_time = calloc(1,sizeof(double));
	short int *number_of_tables = calloc(1,sizeof(short int));
	error_code = readInputPacketFileLine(IP_lookup);
	while(error_code == 0)
	{
		gettimeofday(&start, NULL);
		interface_lookup(IP_lookup,number_of_tables, interface);
		gettimeofday(&end, NULL);
		printOutputLine(*IP_lookup, *interface, &start, &end,searching_time, *number_of_tables);
		*processedPackets = *processedPackets + 1;
		*totalTableAccesses  = *totalTableAccesses + *number_of_tables;
		*totalPacketProcessingTime  = *totalPacketProcessingTime + *searching_time;
		error_code = readInputPacketFileLine(IP_lookup);
	}
	free(IP_lookup);
	free(interface);
	free(searching_time);
	free(number_of_tables);
}
