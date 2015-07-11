##Introduction
本工程为公司职前作业。主要分为2个模块：**dns_server**, **host_util**，**request.sh**

> **dns_server** 是一个简单的DNS模拟服务器，它接受来自客户端的DNS请求，并按照DNS标准格式对请求报文进行解析。由于设计简单，本程序仅响应QTYPEs为A 类的地址请求。 
> #### 特点
> - 将域名-地址信息存储在hosts文件中，方便修改
> - dns_server运行时加载hosts文件，并以hashmap的数据结构进行存储
> - 可以使用dig、nslookup等程序与dns_server进行通信

可以手动编辑hosts文件，也可以使用host_util来增删改查。
> **host_util**程序提供了增加、删除、查询哈希表等的应用接口，操作的结果将会保存在hosts文件中。

**./request.sh** 这个脚本文件用于向dns_server发送请求，其中使用到dig命令。

##Build
	make dns  	生成dns_server可执行文件
	make host 	生成host_util可执行文件
	make clean	删除这些可执行文件
##Test
运行dns_server程序

    ./dns_server
    ============ Waitting For DNS Request on Port 5354 ============
在另一个终端执行脚本文件

    ./request.sh www.tp-link.com	//查询hosts目录中已有的域名
    50.62.149.47
    ./request.sh www.supermax.com	//查询hosts目录中不存在的域名
    DNS request timed out.
  
  查询的同时，dns_server端也会输出报文的情况
  

    ============ Waitting For DNS Request on Port 5354 ============
	Query for 'www.tp-link.com'
	QUERY { ID: 9f2b. FIELDS: [ QR: 1, OpCode: 0 ], QDcount: 1, ANcount: 1, NScount: 0, ARcount: 0, Question { qName 'www.tp-link.com', qType 1, qClass 1 }
	}
	ResourceRecord { name 'www.tp-link.com', type 1, class 1, ttl 3600, rd_length 4, 	Address Resource Record { address 50.62.149.47 }}
	
	Query for 'www.supermax.com'
	QUERY { ID: 62cc. FIELDS: [ QR: 1, OpCode: 0 ], QDcount: 1, ANcount: 0, NScount: 0, ARcount: 0, Question { qName 'ddff', qType 1, qClass 1 }
	}

测试host_util

    ./host_util show               show entire hashmap 	
	./host_util count              how many items			
	./host_util add [domain] [ip]  add a domain-ip pair	
	./host_util rm [domain]        rm a domain-ip pair	
	./host_util search [domain]    search this domain 
## Recommended Reading
[RFC1035](http://www.zytrax.com/books/dns/apd/rfc1035.txt)
[GitHub Source : A Simple DNS Server](https://github.com/mwarning/SimpleDNS)
[GitHub Source : Hashmap](https://github.com/petewarden/c_hashmap)