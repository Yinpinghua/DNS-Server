all: dns host
dns: src/dns_server.c src/dns.c src/hashmap.c inc/dns.h inc/hashmap.h
	gcc src/dns_server.c src/dns.c src/hashmap.c -Iinc -o dns_server
host: src/host_util.c src/hashmap.c inc/hashmap.h
	gcc src/host_util.c src/hashmap.c -Iinc -o host_util 
clean:
	rm dns_server host_util
