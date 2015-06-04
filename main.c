#include <stdlib.h>
#include <unistd.h>  
#include "geolocation.h"

const char * usage = 
"Usage: %s [OPTION] \n"
"Convert a country database from CSV to GeoIP binary format.\n"
"\n"
"-i CSV_FILE  add copyright or other info TEXT to output\n"
"-I TEXT      add copyright or other info TEXT to output\n"
"-o FILE      write the binary data to FILE, not stdout\n"
"-h           display this help and exit\n";


int main(int argc, char *argv[]){  
	int opt;  
	char *info = NULL;
	char *output = NULL;
	char *input = NULL;
	ips_t *ips;
	opterr = 0;  

	while ((opt = getopt(argc, argv, "I:i:o:h"))!=-1)  {  
		switch(opt)  {  
			case 'i':  
				input = optarg;
				break;  
			case 'I':  
				info = optarg;
				break;  
			case 'o':  
				output = optarg;
				break;  
			case 'h':  
				fprintf(stderr, "%s", usage);  
				exit(0);
			default:  
				fprintf(stderr, "%s", usage);  
				exit(1);
		}  
	}
	//if(!(input && output)){
	if(!input){
		fprintf(stderr, "%s", usage);  
		exit(1);
	}

	ips = open_ips(input);
	if(ips == NULL){
		exit(1);
	}
	print_ip(ips, "1.0.167.5");
	print_ip(ips, "1.0.165.5");


	clean_ips(ips);
}  
