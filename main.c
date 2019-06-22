/********************************************************************
 * Routing algorithm based on Two-level hardware multi-bit trie
 *
 * Created by Sergio Maeso Jiménez
 *
 * Universidad Carlos III, Madrid 20 de Febrero de 2015
 *
 ********************************************************************/
#include "main.h"
#include "io.h"
#include "rib.h"

int error_code;		// In this variable we save the error codes that produces some methods in "io.c"

RIB_t rib;

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
  RIB_init(&rib);

	processedPackets  = calloc(1,sizeof(int));
	totalTableAccesses  = calloc(1,sizeof(double));
	totalPacketProcessingTime  = calloc(1,sizeof(double));
	error_code = 0;

	if( argc == 3 )
	{
		error_code = initializeIO(argv[1], argv[2]); //Initialize Input
		if(error_code != 0){
			printf("\nERROR: \n\t");
			printIOExplanationError(error_code);
			return -1;
		}
		initializeFIB();
		compute_routes();
		printSummary(*processedPackets, (*totalTableAccesses / *processedPackets), (*totalPacketProcessingTime / *processedPackets));
		freeIO();			/* Freeing Resources */
    RIB_destroy(&rib);
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
  error_code = readFIBLine(IP_addr, aux_prefixLength, aux_outInterface);
  while(error_code == 0){  //WHILE NOT EOF OR ANOTHER TYPE OF ERROR
    RIB_addRoute(&rib, IP_addr,aux_prefixLength,aux_outInterface);
    error_code = readFIBLine(IP_addr,aux_prefixLength,aux_outInterface);
  }
  free(IP_addr);
  free(aux_prefixLength);
  free(aux_outInterface);
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
		RIB_lookup(&rib, IP_lookup, number_of_tables, interface);
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
