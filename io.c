#include "io.h"

/***********************************************************************
 * Static variables for the input/output files
 ***********************************************************************/
static FILE *routingTable;
static FILE *inputFile;
static FILE *outputFile;


/********************************************************************
 * Initalize file descriptors
 *
 * routingTableName contains FIB info (argv[1] of main function)
 * inputFileName contains IP addresses (argv[2] of main function)
 *
 ***********************************************************************/
int initializeIO(char *routingTableName, char *inputFileName){

	char outputFileName[100];

  routingTable = fopen(routingTableName, "r");
  if (routingTable == NULL) return ROUTING_TABLE_NOT_FOUND;

  inputFile = fopen(inputFileName, "r");
  if (inputFile == NULL) {
  	fclose(routingTable);
   	return INPUT_FILE_NOT_FOUND;
 	}

  sprintf(outputFileName, "%s%s", inputFileName, OUTPUT_NAME);
  outputFile = fopen(outputFileName, "w");
  if (outputFile == NULL) {
    fclose(routingTable);
    fclose(inputFile);
    return CANNOT_CREATE_OUTPUT;
  }

  return OK;

}


/***********************************************************************
 * Close the input/output files
 ***********************************************************************/
void freeIO() {

	fclose(inputFile);
  fclose(outputFile);
  fclose(routingTable);

}


/***********************************************************************
 * Write explanation for error identifier (verbose mode)
 ***********************************************************************/
void printIOExplanationError(int result){

	switch(result) {
    case ROUTING_TABLE_NOT_FOUND:
      printf("Routing table not found\n");
			exit(0);

    case INPUT_FILE_NOT_FOUND:
      printf("Input file not found\n");
			exit(0);

    case BAD_ROUTING_TABLE:
      printf("Bad routing table structure\n");
			exit(0);
    case BAD_INPUT_FILE:
      printf("Bad input file structure\n");
			exit(0);
    case PARSE_ERROR:
      printf("Parse error\n");
			exit(0);
    case CANNOT_CREATE_OUTPUT:
      printf("Cannot create output file\n");
			exit(0);
		case REACHED_EOF:
			printf("Reached End Of File\n");
			exit(0);
    default:
      printf("Unknown error\n");
			exit(0);
  }
	exit(0);

}


/***********************************************************************
 * Read one entry in the FIB
 *
 * It should be noted that prefix, prefixLength and outInterface are
 * pointers since they are used as output parameters
 *
 ***********************************************************************/
int readFIBLine(uint32_t *prefix, int *prefixLength, int *outInterface){
	int n[4], result;

	result = fscanf(routingTable, "%i.%i.%i.%i/%i\t%i\n", &n[0], &n[1], &n[2], &n[3], prefixLength, outInterface);
	if (result == EOF) return REACHED_EOF;
  else if (result != 6) return BAD_ROUTING_TABLE;
  else{
 		//remember that pentium architecture is little endian
 		*prefix = (n[0]<<24) + (n[1]<<16) + (n[2]<<8) + n[3];
		//*prefix = n[0]*pow(2,24) + n[1]*pow(2,16) + n[2]*pow(2,8) + n[3];
		return OK;
	}

}


/***********************************************************************
 * Read one entry in the input packet file
 *
 * Again, it should be noted that IPAddress is a pointer since it is used
 * as output parameter
 *
 ***********************************************************************/
int readInputPacketFileLine(uint32_t *IPAddress){

  int n[4], result;

	result = fscanf(inputFile, "%i.%i.%i.%i\n", &n[0], &n[1], &n[2], &n[3]);
	if (result == EOF) return REACHED_EOF;
  else if (result != 4) return BAD_INPUT_FILE;
  else{
 		//remember that pentium architecture is little endian
  	*IPAddress = (n[0]<<24) + (n[1]<<16) + (n[2]<<8) + n[3];
		//*IPAddress = n[0]*pow(2,24) + n[1]*pow(2,16) + n[2]*pow(2,8) + n[3];
		return OK;
	}

}


/***********************************************************************
 * Print a line to the output file
 *
 * gettimeofday(&initialTime, NULL) must be called right before the lookup function
 *
 * gettimeofday(&finalTime, NULL) must be called right after the lookup function
 *
 * The lookup function must return (either as output parameter or as return value)
 * the number of hash tables that have been accessed for every IP address
 *
 ***********************************************************************/
 void printOutputLine(uint32_t IPAddress, int outInterface, struct timeval *initialTime, struct timeval *finalTime,
                        double *searchingTime, int numberOfHashtables) {

  unsigned long sec, usec;

  usec = finalTime->tv_usec - initialTime->tv_usec;
  if (usec > finalTime->tv_usec) initialTime->tv_sec += 1;
  sec = finalTime->tv_sec - initialTime->tv_sec;

  *searchingTime = 1000000*sec + usec;

	//remember that output interface equals 0 means no matching
	//remember that if no matching but default route is specified in the FIB, the default output interface
	//must be stored to avoid dropping the packet (i.e., MISS)
  if (!outInterface){
    fprintf(outputFile,"%i.%i.%i.%i;%s;%i;%.0lf\n",IPAddress >> 24, (IPAddress >> 16) & 0x000000ff, (IPAddress >> 8) & 0x000000ff, IPAddress & 0x000000ff , "MISS",numberOfHashtables, *searchingTime);
	}
	else{
		fprintf(outputFile,"%i.%i.%i.%i;%i;%i;%.0lf\n",IPAddress >> 24, (IPAddress >> 16) & 0x000000ff, (IPAddress >> 8) & 0x000000ff, IPAddress & 0x000000ff , outInterface,numberOfHashtables, *searchingTime);
	}
}


/***********************************************************************
 * Print execution summary to the output file
 *
 * It should be noted that:
 *
 * 		averageTableAccesses = totalTableAccesses/processedPackets
 *
 *		averagePacketProcessingTime = totalPacketProcessingTime/processedPackets
 *
 ***********************************************************************/
void printSummary(int processedPackets, double averageTableAccesses, double averagePacketProcessingTime){
	fprintf(outputFile, "\nPackets processed= %i\n", processedPackets);
  fprintf(outputFile, "Average table accesses= %.2lf\n", averageTableAccesses);
  fprintf(outputFile,"Average packet processing time (usecs)= %.2lf\n", averagePacketProcessingTime);
	printMemoryTimeUsage();

}


/***********************************************************************
 * Print memory and CPU time
 *
 * For more info: man getrusage
 *
 ***********************************************************************/
void printMemoryTimeUsage(){

	float    user_time, system_time;
  long int memory;
  struct rusage usage;

  if (getrusage (RUSAGE_SELF, &usage)){
    printf("Resource measurement failed.\n");
  }
  else{
  	user_time = (float)usage.ru_utime.tv_sec+(float)usage.ru_utime.tv_usec/1000000;
  	system_time  = (float)usage.ru_stime.tv_sec+(float)usage.ru_stime.tv_usec/1000000;
  	memory = usage.ru_maxrss;

  	fprintf(outputFile, "Memory (Kbytes) = %ld\n", memory );
  	fprintf(outputFile, "CPU Time (secs)= %.6f\n\n", user_time+system_time);
  }

}
