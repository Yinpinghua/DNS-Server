/*
 *	Generic DNS implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dns.h"

//============= Deconding functions =============

int decode_msg(struct Message* msg, const uchar* buffer, int size)
{
	char name[300];
	int i;

	decode_header(msg, &buffer);

	if((msg->anCount+msg->nsCount+msg->arCount) != 0)
	{
		printf("Only questions expected!\n");
		return -1;
	}

	// parse questions
	uint qcount = msg->qdCount;
	struct Question* qs = msg->questions;
	for(i = 0; i < qcount; ++i)
	{
		struct Question* q = malloc(sizeof(struct Question));

		q->qName = decode_domain_name(&buffer);
		q->qType = get16bits(&buffer);
		q->qClass = get16bits(&buffer);

		q->next = qs;
		msg->questions = q;
	}

	// We do not expect any resource records to parse here.

	return 0;
}

void decode_header(struct Message* msg, const uchar** buffer)
{
	msg->id = get16bits(buffer);

	uint fields = get16bits(buffer);
	msg->qr = (fields & QR_MASK) >> 15;
	msg->opcode = (fields & OPCODE_MASK) >> 11;
	msg->aa = (fields & AA_MASK) >> 10;
	msg->tc = (fields & TC_MASK) >> 9;
	msg->rd = (fields & RD_MASK) >> 8;
	msg->ra = (fields & RA_MASK) >> 7;
	msg->rcode = (fields & RCODE_MASK) >> 0;

	msg->qdCount = get16bits(buffer);
	msg->anCount = get16bits(buffer);
	msg->nsCount = get16bits(buffer);
	msg->arCount = get16bits(buffer);
}

// 3foo3bar3com0 => foo.bar.com
char* decode_domain_name(const uchar** buffer)
{
	uchar name[256];
	const uchar* buf = *buffer;
	int j = 0;
	int i = 0;
	while(buf[i] != 0)
	{
		//if(i >= buflen || i > sizeof(name))
		//	return NULL;
		
		if(i != 0)
		{
			name[j] = '.';
			j += 1;
		}

		int len = buf[i];
		i += 1;

		memcpy(name+j, buf+i, len);
		i += len;
		j += len;
	}

	name[j] = '\0';

	*buffer += i + 1; //also jump over the last 0

	return strdup(name);
}

//============= Enconding functions =============

int encode_msg(struct Message* msg, uchar** buffer)
{
	struct Question* q;
	int rc;

	encode_header(msg, buffer);

	q = msg->questions;
	while(q)
	{
		encode_domain_name(buffer, q->qName);
		put16bits(buffer, q->qType);
		put16bits(buffer, q->qClass);

		q = q->next;
	}

	rc = 0;
	rc |= encode_resource_records(msg->answers, buffer);
	rc |= encode_resource_records(msg->authorities, buffer);
	rc |= encode_resource_records(msg->additionals, buffer);

	return rc;
}

void encode_header(struct Message* msg, uchar** buffer)
{
	put16bits(buffer, msg->id);

	int fields = 0;
	fields |= (msg->qr << 15) & QR_MASK;
	fields |= (msg->rcode << 0) & RCODE_MASK;
	// TODO: insert the rest of the fields
	put16bits(buffer, fields);

	put16bits(buffer, msg->qdCount);
	put16bits(buffer, msg->anCount);
	put16bits(buffer, msg->nsCount);
	put16bits(buffer, msg->arCount);
}

// foo.bar.com => 3foo3bar3com0
void encode_domain_name(uchar** buffer, const uchar* domain)
{
	uchar* buf = *buffer;
	const uchar* beg = domain;
	const uchar* pos;
	int len = 0;
	int i = 0;

	while(pos = strchr(beg, '.'))
	{
		len = pos - beg;
		buf[i] = len;
		i += 1;
		memcpy(buf+i, beg, len);
		i += len;

		beg = pos + 1;
	}

	len = strlen(domain) - (beg - domain);

	buf[i] = len;
	i += 1;

	memcpy(buf + i, beg, len);
	i += len;

	buf[i] = 0;
	i += 1;

	*buffer += i;
}

int encode_resource_records(struct ResourceRecord* rr, uchar** buffer) 
{
	int i;
	while(rr)
	{
		// Answer questions by attaching resource sections.
		encode_domain_name(buffer, rr->name);
		put16bits(buffer, rr->type);
		put16bits(buffer, rr->class);
		put32bits(buffer, rr->ttl);
		put16bits(buffer, rr->rd_length);
		
		switch(rr->type)
		{
			case A_Resource_RecordType:
				for(i = 0; i < 4; ++i)
					put8bits(buffer, rr->rd_data.a_record.addr[i]);
				break;
			default:
				fprintf(stderr, "Unknown type %u. => Ignore resource record.\n", rr->type);
				return 1;
		}
		
		rr = rr->next;
	}
	return 0;
}

//============= Resolve functions =============

// For every question in the message add a appropiate resource record
// in either section 'answers', 'authorities' or 'additionals'.
void resolver_process(struct Message* msg, map_t mymap)
{
	struct ResourceRecord* beg;
	struct ResourceRecord* rr;
	struct Question* q;
	int rc;

	// leave most values intact for response
	msg->qr = 1; // this is a response
	msg->aa = 1; // this server is authoritative
	msg->ra = 0; // no recursion available
	msg->rcode = Ok_ResponseType;

	//should already be 0
	msg->anCount = 0;
	msg->nsCount = 0;
	msg->arCount = 0;

	//for every question append resource records
	q = msg->questions;
	while(q)
	{
		rr = malloc(sizeof(struct ResourceRecord));

		rr->name = strdup(q->qName);
		rr->type = q->qType;
		rr->class = q->qClass;
		rr->ttl = 60*60; //in seconds; 0 means no caching
		
		printf("Query for '%s'\n", q->qName);
		
		// We only can only answer one question types so far
		// and the answer (resource records) will be all put
		// into the answers list.
		// This behavior is probably non-standard!
		switch(q->qType)
		{
			case A_Resource_RecordType:
				rr->rd_length = 4;
				rc = get_A_Record(rr->rd_data.a_record.addr, q->qName, mymap);
				if(rc < 0){
					 goto next;
				}
				break;
			/*
			case AAAA_Resource_RecordType:
			case NS_Resource_RecordType:
			case CNAME_Resource_RecordType:
			case SOA_Resource_RecordType:
			case PTR_Resource_RecordType:
			case MX_Resource_RecordType:
			case TXT_Resource_RecordType:
			*/
			default:
				msg->rcode = NotImplemented_ResponseType;
				printf("Cannot answer question of type %d.\n", q->qType);
				goto next;
		}

		msg->anCount++;

		// prepend resource record to answers list
		beg = msg->answers;
		msg->answers = rr;
		rr->next = beg;

		//jump here to omit question
		next:
		
		// process next question
		q = q->next;
	}
}

// return IPv4 address
int get_A_Record(uchar addr[4], char domain_name[], map_t mymap)
{
	data_struct_t *hostAddr;
	int status;
	hostAddr = (data_struct_t *)malloc(sizeof(data_struct_t));

	status = hashmap_get(mymap, domain_name, (void**)(&hostAddr));
	if(status == MAP_OK){
		sscanf(hostAddr->addr, "%d.%d.%d.%d", 
			(int *)&addr[0], (int *)&addr[1], (int *)&addr[2], (int *)&addr[3]);
		return 0;
	}
	else
		return -1;
}

// ============= Basic memory operations =============

size_t get16bits(const uchar** buffer)
{
	ushort value;

	value = ntohs( *((typeof(value)*) *buffer) );
	*buffer += 2;

	return value;
}

void put8bits(uchar** buffer, uchar value)
{
	*((typeof(value)*) *buffer) = value;
	*buffer += 1;
}

void put16bits(uchar** buffer, ushort value)
{
	*((typeof(value)*) *buffer) = htons( value );
	*buffer += 2;
}

void put32bits(uchar** buffer, unsigned long long value)
{
	*((typeof(value)*) *buffer) = htonl( value );
	*buffer += 4;
}

// ============= Debug functions =============

void print_query(struct Message* msg)
{
	printf("QUERY { ID: %02x", msg->id);
	printf(". FIELDS: [ QR: %u, OpCode: %u ]", msg->qr, msg->opcode);
	printf(", QDcount: %u", msg->qdCount);
	printf(", ANcount: %u", msg->anCount);
	printf(", NScount: %u", msg->nsCount);
	printf(", ARcount: %u,\n", msg->arCount);

	struct Question* q = msg->questions;

	printf("  Question { qName '%s', qType %u, qClass %u }\n",
		q->qName,
		q->qType,
		q->qClass
	);

	printf("}\n");
}

void print_resource_record(struct ResourceRecord* rr)
{
	int i;
	while(rr)
	{
		printf("  ResourceRecord { name '%s', type %u, class %u, ttl %u, rd_length %u, ",
				rr->name,
				rr->type,
				rr->class,
				rr->ttl,
				rr->rd_length
		);

		union ResourceData *rd = &rr->rd_data;
		switch(rr->type)
		{
			case A_Resource_RecordType:
				printf("Address Resource Record { address ");
			
				for(i = 0; i < 4; ++i)
					printf("%s%u", (i ? "." : ""), rd->a_record.addr[i]);
			
				printf(" }");
				break;
			case NS_Resource_RecordType:
				printf("Name Server Resource Record { name %s}",
					rd->name_server_record.name
				);
				break;
			case CNAME_Resource_RecordType:
				printf("Canonical Name Resource Record { name %u}",
					rd->cname_record.name
				);
				break;
			case SOA_Resource_RecordType:
				printf("SOA { MName '%s', RName '%s', serial %u, refresh %u, retry %u, expire %u, minimum %u }",
					rd->soa_record.MName,
					rd->soa_record.RName,
					rd->soa_record.serial,
					rd->soa_record.refresh,
					rd->soa_record.retry,
					rd->soa_record.expire,
					rd->soa_record.minimum
				);
				break;
			case PTR_Resource_RecordType:
				printf("Pointer Resource Record { name '%s' }",
					rd->ptr_record.name
				);
				break;
			case MX_Resource_RecordType:
				printf("Mail Exchange Record { preference %u, exchange '%s' }",
					rd->mx_record.preference,
					rd->mx_record.exchange
				);
				break;
			case TXT_Resource_RecordType:
				printf("Text Resource Record { txt_data '%s' }",
					rd->txt_record.txt_data
				);
				break;
			case AAAA_Resource_RecordType:
				printf("AAAA Resource Record { address ");
			
				for(i = 0; i < 16; ++i)
					printf("%s%02x", (i ? ":" : ""), rd->aaaa_record.addr[i]);
			
				printf(" }");
				break;
			default:
				printf("Unknown Resource Record { ??? }");
		}
		printf("}\n");
		rr = rr->next;
	}
}