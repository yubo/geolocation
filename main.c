#include <stdlib.h>
#include <unistd.h>  
#include "geolocation.h"

const char * usage = 
"Usage: %s [OPTION] \n"
"Convert a country database from CSV to GeoIP binary format.\n"
"\n"
"-i ip_sample.txt  input file\n"
"-a           use alias\n"
"-h           display this help and exit\n";


int main(int argc, char *argv[]){  
	int opt;  
	char *input = NULL;
	uint32_t flags = 0;
	ips_t *ips;
	opterr = 0;  

	while ((opt = getopt(argc, argv, "i:o:ha"))!=-1)  {  
		switch(opt)  {  
			case 'i':  
				input = optarg;
				break;  
			case 'a':  
				flags |= GEO_F_ALIAS;
				break;  
			case 'h':  
				fprintf(stderr, usage, argv[0]);
				exit(0);
			default:  
				fprintf(stderr, usage, argv[0]);
				exit(1);
		}  
	}
	//if(!(input && output)){
	if(!input){
		fprintf(stderr, usage, argv[0]);
		exit(1);
	}

	ips = open_ips(input, flags);
	if(ips == NULL){
		exit(1);
	}
	print_ip(ips, "1.0.167.5");
	print_ip(ips, "1.0.165.5");
	print_ip(ips, "1.0.8.5");

	clean_ips(ips);
}  
