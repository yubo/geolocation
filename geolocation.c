#define _XOPEN_SOURCE

#include <unistd.h>  
#include <stdlib.h>
#include <stdio.h>  
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "radix_tree.h"

#define MAX_CSV_LINE 2774714 
#define MAX_TEXT_SIZE  200000000
#define ALL_ONES (~(uint32_t)0)
#define _BSD_SOURCE

#define DEBUG
const char * usage = 
"Usage: %s [OPTION] \n"
"Convert a country database from CSV to GeoIP binary format.\n"
"\n"
"-i CSV_FILE  add copyright or other info TEXT to output\n"
"-I TEXT      add copyright or other info TEXT to output\n"
"-o FILE      write the binary data to FILE, not stdout\n"
"-h           display this help and exit\n";

#define DPRINTF(format, ...) fprintf(stderr, "%s(%d): " format, __func__, __LINE__, ## __VA_ARGS__)
#define D(format, ...) do { \
	DPRINTF(format, ##__VA_ARGS__); \
} while (0)

typedef struct _ip_entry{
	uint32_t min;
	uint32_t max;
	char min_addr[16];
	char max_addr[16];
	char country[64];
	char province[64];
	char city[64];
	char village[64];
	char isp[64];
	uintptr_t offset;
}_ip_entry;

typedef struct ip_entry{
#ifdef DEBUG
	uint32_t min;
	uint32_t max;
#endif
    uintptr_t country;
    uintptr_t province;
    uintptr_t city;
    uintptr_t village;
    uintptr_t isp;
}ip_entry;


typedef struct ips_t{
	int e_len;
	int t_len;
	char  t[MAX_TEXT_SIZE];
	ip_entry e[MAX_CSV_LINE];
	radix_tree_t *tree;
}ips_t;


int count_righthand_zero_bit(uint32_t number, int bits){
	int i;
	if(number == 0){
		return bits; 
	}
	for(i = 0; i < bits; i++){
		if ((number >> i) % 2 )
			return i;
	}
	return bits;
}

int get_prefix_length(uint32_t n1, uint32_t n2, int bits){
	int i;
	for(i = 0; i < bits; i++){
		if (n1 >> i == n2 >> i )
			return bits - i;
	}
	return 0;
}

char *u32toa(uint32_t u){
	return inet_ntoa((*(struct in_addr*)&u));
}

char *_u32toa(uint32_t u){
	static char buff[16];
	sprintf(buff,"%s", inet_ntoa((*(struct in_addr*)&u)));
	return buff;
}

char *__u32toa(uint32_t u){
	static char buff[16];
	sprintf(buff,"%s", inet_ntoa((*(struct in_addr*)&u)));
	return buff;
}

void insert(radix_tree_t *tree, uint32_t min, uint32_t max, int prefix, uintptr_t value){
	//radix tree store network byte order
	int ret;
	uint32_t mask = (uint32_t)(0xffffffffu <<(32 - prefix));
	uint32_t addr = min & mask;
	ret = radix32tree_insert(tree, addr, mask, value);
}

void set_range(radix_tree_t *tree, uint32_t min, uint32_t max, uintptr_t leaf) {
	// add_min and add_max must be host byte order
	uint32_t _min = min;
	uint32_t current, addend;
	int nbits, prefix;
	while(min <= max){
		nbits = count_righthand_zero_bit(min, 32);
		current = 0;
		while (nbits >= 0) {
			addend = pow(2, nbits) - 1;
			current = min + addend;
			nbits -= 1;
			if (current <= max)
				break;
		}
		prefix = get_prefix_length(min, current, 32);
		insert(tree, _min, current, prefix, leaf);
		if(current == ALL_ONES)
			break;
		min = current + 1;
		_min = min;
	}
}



#ifdef DEBUG
void dump_es(ips_t *ips){
	int i;
	if(ips){
		for(i = 0; i < ips->e_len; i++){
			D("[%d] range(%u, %u) ip(%s, %s) country[%s], province[%s], city[%s], village[%s], isp[%s]\n", 
					i, ips->e[i].min, ips->e[i].max, u32toa(htonl(ips->e[i].min)), _u32toa(htonl(ips->e[i].max)), 
					ips->t+ips->e[i].country, ips->t+ips->e[i].province, ips->t+ips->e[i].city, 
					ips->t+ips->e[i].village, ips->t+ips->e[i].isp );
		}
	}
}
#else
#define dump_es(a)
#endif

ips_t * open_ips(char *filename){
	FILE *fp;
	int num;
	char line[1024];
	ips_t *ips;
	_ip_entry _e;
	ip_entry *e;

	if((fp = fopen(filename, "r")) == NULL){
		D("fopen %s error\n", filename);
		return NULL;
	}

	ips = calloc(1, sizeof(ips_t));
	if(ips == NULL){
		D("calloc error\n");
		goto out;
	}
	ips->t_len = 1;
	ips->e_len = 0;

	if((ips->tree =  radix_tree_create()) == NULL){
		goto err_alloc_ips;
	}

	for (; fgets(line, sizeof(line), fp);) {
		if(ips->e_len == MAX_CSV_LINE){
			D("MAX_CSV_LINE(%d) not enough\n", MAX_CSV_LINE);
			goto err_alloc_tree;
		}
		e = &ips->e[ips->e_len];
		num = sscanf(line, "%u,%u,%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,\n]", 
				&_e.min, &_e.max, _e.min_addr, _e.max_addr, 
				_e.country, _e.province, _e.city, _e.village, _e.isp);
		if(num < 9 ){
			D("SKIP[%d] %s\n",num, line);
			continue;
		}
		ips->e_len++;

#ifdef DEBUG
		e->min = _e.min;
		e->max = _e.max;
#endif

#define _APPEND(a) do{ \
		if(strlen(_e.a)>2){ \
			e->a = ips->t_len; \
			ips->t_len += sprintf(ips->t + ips->t_len, "%s", &_e.a[1]); \
			ips->t[ips->t_len-1] = '\0'; \
		}else{ \
			e->a = 0; \
		} \
}while(0)
		_APPEND(country);
		_APPEND(province);
		_APPEND(city);
		_APPEND(village);
		_APPEND(isp);
		set_range(ips->tree, _e.min, _e.max, (uintptr_t)e);
	}

out:
	fclose(fp);
	return ips;

err_alloc_tree:
	radix_tree_clean(ips->tree);
err_alloc_ips:
	free(ips);
	fclose(fp);
	return NULL;
}

void clean_ips(ips_t *ips){
	radix_tree_clean(ips->tree);
	free(ips);
	ips = NULL;
}

void print_ip(ips_t *ips, char *ip){
	ip_entry *e;
	struct in_addr add;
	int ret;
	ret = inet_aton(ip, &add);
	e = (ip_entry *)radix32tree_find(ips->tree, ntohl(add.s_addr));
	D("ip[%s], country[%s], province[%s], city[%s], village[%s], isp[%s]\n", ip,
					ips->t+e->country, ips->t+e->province, ips->t+e->city, 
					ips->t+e->village, ips->t+e->isp );
}

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
	print_ip(ips, "1.4.194.5");

	clean_ips(ips);
}  
