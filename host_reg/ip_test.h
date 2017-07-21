#include "pcap.h"  
#include <winsock2.h>  
#include <stdio.h>
#include <stdlib.h>
#include <conio.h> 
#include <ntddndis.h> 
#include	<map>
#include	<list>
#include	<algorithm>
#include	<time.h>
#include <fstream>
#include <iostream>
#include "tinyxml.h"	
#include <windows.h>
#include <atlstr.h>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <cctype>
#include <iomanip>
#include	<regex>
#include "cross_platform.h"
#include "log.h"
#include<queue>

#pragma comment(lib,"wpcap.lib")  
#pragma comment(lib,"packet.lib")  
#pragma comment(lib,"ws2_32.lib")  


#define Ether_addr_len 6
#define Ether_IP 0x0800
#define TCP_Protocal 0x0600
#define Max_Adapter_Num 10
#define Max_pos_Num 256*256*256
#define SHIFT 5
#define MASK 0x1F
#define Ether_Vlan 0x8100
#define queueSize 5000 

//ip_list_table empty;
//typedef bitset<bit_Num> ipa;
//typedef map<INT, ipa> bitmap;


// ethernet header
typedef struct ether_header{
	UCHAR ether_source_addr[Ether_addr_len];
	UCHAR ether_destin_addr[Ether_addr_len];
	USHORT  ether_typed;
}ether_header;

// ip address
typedef struct ip_address {
	UCHAR byte1;
	UCHAR byte2;
	UCHAR byte3;
	UCHAR byte4;
}ip_address;

//typedef map<ip_address, INT> ip_list_table;

// IP header
typedef struct IP_header{
	UCHAR ver_and_len;
	UCHAR service_type;
	USHORT total_length;
	USHORT identi;
	USHORT flag_frag;
	UCHAR ttl;
	UCHAR protocol;
	USHORT headercheck;
	ip_address source_ip;
	ip_address dest_ip;
}IP_header;

// TCP header 
typedef struct TCP_header {
	USHORT source_port;
	USHORT dest_port;
	ULONG squence_num;
	ULONG ack_num;
	USHORT flags;
	USHORT window_size;
	USHORT checksum;
	USHORT urgent_point;
}TCP_header;


//
//class IPmatch
//{
//public:
//	IPmatch(ip_address ip) :byteset(ip){};
//	bool operator () ( const ip_address &ip) const;
//private:
//	ip_address byteset;
//};
//bool IPmatch::operator() (const ip_address &ip) const
//{
//	return  (byteset.byte1 == ip.byte1) && (byteset.byte2 == ip.byte2) && (byteset.byte3 == ip.byte3);
//}

class MACMATCH
{
public:
	MACMATCH(const std::string &MACList) :MACQuery(MACList){}
	bool operator ()(const std::map<std::string, std::string>::value_type &MACList)
	{
		return MACList.second == MACQuery;
	}
private:
	const std::string &MACQuery;
};


template<typename DataT>
class IProducerConsumer
{
public:
	virtual void Run() = 0;
	virtual bool AddTask(DataT data) = 0;
};


//time
static time_t now;
static time_t justnow;
unsigned int bitmap[Max_pos_Num / sizeof(unsigned int)];
