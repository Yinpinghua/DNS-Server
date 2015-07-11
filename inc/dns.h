
#ifndef __DNS_H__
#define __DNS_H__

#include <stdint.h>
#include "hashmap.h"

#define MAX_LENGTH 256
#define ADDR_LENGTH 32

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

// ============= Masks and constants =============

static const uint QR_MASK = 0x8000;
static const uint OPCODE_MASK = 0x7800;
static const uint AA_MASK = 0x0400;
static const uint TC_MASK = 0x0200;
static const uint RD_MASK = 0x0100;
static const uint RA_MASK = 0x8000;
static const uint RCODE_MASK = 0x000F;

// Response Type 
enum {
	Ok_ResponseType = 0,
	FormatError_ResponseType = 1,
	ServerFailure_ResponseType = 2,
	NameError_ResponseType = 3,
	NotImplemented_ResponseType = 4,
	Refused_ResponseType = 5
};

// Resource Record Types
enum {
	A_Resource_RecordType = 1,
	NS_Resource_RecordType = 2,
	CNAME_Resource_RecordType = 5,
	SOA_Resource_RecordType = 6,
	PTR_Resource_RecordType = 12,
	MX_Resource_RecordType = 15,
	TXT_Resource_RecordType = 16,
	AAAA_Resource_RecordType = 28,
	SRV_Resource_RecordType = 33
};

// Operation Code
enum {
	QUERY_OperationCode = 0, /* standard query */
	IQUERY_OperationCode = 1, /* inverse query */
	STATUS_OperationCode = 2, /* server status request */
	NOTIFY_OperationCode = 4, /* request zone transfer */
	UPDATE_OperationCode = 5 /* change resource records */
};

// Response Code
enum {
	NoError_ResponseCode = 0,
	FormatError_ResponseCode = 1,
	ServerFailure_ResponseCode = 2,
	NameError_ResponseCode = 3
};

// Query Type 
enum {
	IXFR_QueryType = 251,
	AXFR_QueryType = 252,
	MAILB_QueryType = 253,
	MAILA_QueryType = 254,
	STAR_QueryType = 255
};

// ============= Types =============

typedef struct data_struct_s
{
    char hostname[MAX_LENGTH];
    char addr[ADDR_LENGTH] ;
} data_struct_t;

// part of struct Message : Question Section
struct Question {
	char *qName;
	uint16_t qType;
	uint16_t qClass;
	struct Question* next; // for linked list
};

// part of struct ResourceRecord
union ResourceData {
	struct {
		char *txt_data;
	} txt_record;
	struct {
		uchar addr[4];
	} a_record;
	struct {
		char* MName;
		char* RName;
		uint serial;
		uint refresh;
		uint retry;
		uint expire;
		uint minimum;
	} soa_record;
	struct {
		char *name;
	} name_server_record;
	struct {
		char name;
	} cname_record;
	struct {
		char *name;
	} ptr_record;
	struct {
		ushort preference;
		char *exchange;
	} mx_record;
	struct {
		uchar addr[16];
	} aaaa_record;
	struct {
		ushort priority;
		ushort weight;
		ushort port;
		char *target;
	} srv_record;
};

// part of struct Message : Resource Record Section 
struct ResourceRecord {
	char *name;
	uint16_t type;
	uint16_t class;
	uint16_t ttl;
	uint16_t rd_length;
	union ResourceData rd_data;
	struct ResourceRecord* next; // for linked list
};

// standard DNS message format: refer to < rfc1035 : page 25>
struct Message {
	uint16_t id; /* Identifier */

	/* Flags */
	uint8_t qr; /* Query/Response Flag */
	uint8_t opcode; /* Operation Code */
	uint8_t aa; /* Authoritative Answer Flag */
	uint8_t tc; /* Truncation Flag */
	uint8_t rd; /* Recursion Desired */
	uint8_t ra; /* Recursion Available */
	uint8_t rcode; /* Response Code */

	uint16_t qdCount; /* Question Count */
	uint16_t anCount; /* Answer Record Count */
	uint16_t nsCount; /* Authority Record Count */
	uint16_t arCount; /* Additional Record Count */

	/* At least one question; questions are copied to the response 1:1 */
	struct Question* questions;

	/*
	* Resource records to be send back.
	* Every resource record can be in any of the following places.
	* But every place has a different semantic.
	*/
	struct ResourceRecord* answers;
	struct ResourceRecord* authorities;
	struct ResourceRecord* additionals;
};


// ============= Functions =============

int decode_msg(struct Message* msg, const uchar* buffer, int size);
void decode_header(struct Message* msg, const uchar** buffer);
char* decode_domain_name(const uchar** buffer);

int encode_msg(struct Message* msg, uchar** buffer);
void encode_header(struct Message* msg, uchar** buffer);
void encode_domain_name(uchar** buffer, const uchar* domain);
int encode_resource_records(struct ResourceRecord* rr, uchar** buffer);

void resolver_process(struct Message* msg, map_t mymap);
int get_A_Record(uchar addr[4], char domain_name[], map_t mymap);

size_t get16bits(const uchar** buffer);
void put8bits(uchar** buffer, uchar value);
void put16bits(uchar** buffer, ushort value);
void put32bits(uchar** buffer, unsigned long long value);

void print_query(struct Message* msg);

#endif