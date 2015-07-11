#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "dns.h"

#define DEBUG 0

#define BUFFER_SIZE 2048


int main(int argc, char **argv)
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


	int srvSock;
	int srvPort = 5354;
	
	struct sockaddr_in srvAddr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	// init sockaddr
	memset(&srvAddr, 0, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvAddr.sin_port = htons(srvPort);

	// create server socket
	if( (srvSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1 ){
		printf("create socket error: %s (errno: %d)\n", strerror(errno), errno);
		return -1;
	}

	// bind server socket to srvAddr
	if( bind(srvSock, (struct sockaddr *)&srvAddr, addr_len) == -1){
		printf("bind socket error: %s (errno: %d)\n", strerror(errno), errno);
		return -1;
	}

	printf("============ Waitting For DNS Request on Port %d ============\n", srvPort);

	struct Message msg;
	int recvLen;
	uchar buffer[BUFFER_SIZE];
	struct sockaddr_in clientAddr;

	while(1){
		// clear message
		memset(&msg, 0, sizeof(struct Message));

		// receive request from client
		if( (recvLen = recvfrom(srvSock, buffer, sizeof(buffer), 0, 
			(struct sockaddr *)&clientAddr, &addr_len)) == -1 ){
			close(srvSock);
			printf("recv data error: %s (errno: %d)\n", strerror(errno), errno);
		}

		// decode DNS message from buffer
		if(decode_msg(&msg, buffer, recvLen) != 0) {
			printf("decode message error.\n");
			continue;
		}

		resolver_process(&msg, mymap);

		// Print response 
		print_query(&msg);
		print_resource_record(msg.answers);

		uchar *p = buffer;
		if(encode_msg(&msg, &p) != 0) {
			printf("encode message error.\n");
			continue;
		}

		int buflen = p - buffer;
		sendto(srvSock, buffer, buflen, 0, (struct sockaddr *)&clientAddr, addr_len);
	}
}