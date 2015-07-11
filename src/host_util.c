/*
 * basic operation to file "hosts" implemented by hashmap.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hashmap.h"

#define MAX_LENGTH 256
#define ADDR_LENGTH 32

typedef struct data_struct_s
{
    char hostname[MAX_LENGTH];
    char addr[ADDR_LENGTH] ;
} data_struct_t;


int show(any_t item, void * data_)
{
	item = (int *)item;
	data_struct_t *data = (data_struct_t *)data_ ;
	printf("hostname: %s \taddr: %s \n", data->hostname, data->addr);
	return 0;
}

int print_to_file(any_t fp, void *data_)
{
	fp = (FILE *)fp;
	data_struct_t *data = (data_struct_t *)data_ ;
	fprintf(fp, "%s\t%s\n", data->hostname, data->addr);
	return 0;
}

void help()
{
	printf("usage: \n"
			"./host_util show               show entire hashmap 	\n"
			"./host_util count              how many items			\n"
			"./host_util add [domain] [ip]  add a domain-ip pair	\n"
			"./host_util rm [domain]        rm a domain-ip pair	\n"
			"./host_util search [domain]    search this domain 	\n" );
}


int main(int argc, char* argv[])
{
	int error;
	map_t mymap;
	data_struct_t *hostAddr;
	FILE *fp;
	char hostname[MAX_LENGTH];
	char addr[ADDR_LENGTH];

	// create an empty hashmap
	mymap = hashmap_new();
	if( mymap == NULL )	{ printf("create hashmap failed."); return -1;}
	
	fp = fopen("hosts", "r");	
	if( fp == NULL )	{ printf("hosts doesn't exist."); return -1;}

	// load domain-ip from file "hosts"
	while( fscanf(fp, "%s\t%s", hostname, addr) != EOF ){
		hostAddr = (data_struct_t *)malloc(sizeof(data_struct_t));
		strcpy(hostAddr->hostname, hostname);
		strcpy(hostAddr->addr, addr);
		error = hashmap_put(mymap, hostAddr->hostname, hostAddr);
		assert( error==MAP_OK );
	}

	if(argc < 2){
		help();
		return -1;
	}

	// some operations
	if(!strcmp(argv[1], "show")){
		int item = 0;
		error = hashmap_iterate(mymap, show, (void *)&item);
		assert(error==MAP_OK);
	}
	else if(!strcmp(argv[1], "count")){
		printf("hashmap items' number: %d \n", hashmap_length(mymap));
	}
	else if(!strcmp(argv[1], "add")){
		hostAddr = (data_struct_t *)malloc(sizeof(data_struct_t));
		strcpy( hostAddr->hostname, argv[2]);
		strcpy( hostAddr->addr, argv[3]);
		error = hashmap_put(mymap, hostAddr->hostname, hostAddr);
		fp = freopen("hosts", "a", fp);
		fprintf(fp, "%s\t%s\n", hostAddr->hostname, hostAddr->addr);
		printf("add success. \n");
	}
	else if(!strcmp(argv[1], "rm")){
        error = hashmap_remove(mymap, argv[2]);
        assert(error==MAP_OK);
		fp = freopen("hosts", "w", fp);
		error = hashmap_iterate(mymap, print_to_file, (void *)fp);
		assert(error==MAP_OK);
		printf("rm success. \n");
	}
	else if(!strcmp(argv[1], "search")){
		error = hashmap_get(mymap, argv[2], (void**)(&hostAddr));
		if(error == MAP_OK)
			printf("addr: %s \n", hostAddr->addr);
		else
			printf("not found.\n");
	}
	else
		help();

	// destroy
	fclose(fp);
	hashmap_free(mymap);

	return 0;
}