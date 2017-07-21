#include "ip_test.h"
#include "DumpFile.h"
using namespace std;

char AdapterNameList[Max_Adapter_Num][1024];

typedef struct XMLinfo{
	string filename;
	map<string, INT> HostList;
	std::regex URI;
	std::regex HostName;
	csLock data_mutex;
}XMLinfo;

vector<XMLinfo> info_Table;

typedef struct GroupPrarm{
	string  device_name;
	vector <XMLinfo> *info_Table;
	UINT pthreadnum;
}GroupPrarm;

// adapter name list
char AdapterNameBuff[1024];
string mac_ad;
map<string, string> MAC_List;

//path name
static string AppPath;

//int to string
string int2string(const int n)

{
	std::stringstream newstr;
	newstr << n;
	return newstr.str();
}

// char to hex
string bytestohexstring(UCHAR* bytes, int bytelength)
{
	string str("");
	string str2("0123456789abcdef");
	for (int i = 0; i<bytelength; i++) {
		int b;
		b = 0x0f & (bytes[i] >> 4);
		char s1 = str2.at(b);
		str.append(1, str2.at(b));
		b = 0x0f & bytes[i];
		str.append(1, str2.at(b));
		char s2 = str2.at(b);
	}
	return str;
}

// load xml file
BOOL ReadXmlFile(const string &filename, vector< XMLinfo>* info_Table, vector<string> *MACQuery)
{
	//LOG_FUNC_BEGIN(ReadXmlFile);
	TiXmlDocument *MyDocument = new TiXmlDocument();
	if (!MyDocument->LoadFile(filename.c_str()))
		return FALSE;

	TiXmlElement *Rootelement = MyDocument->RootElement();

	string buf;
	XMLinfo DATA;

	// get host info
	TiXmlElement *ele = Rootelement->FirstChildElement();
	if (ele)
	{			
		TiXmlElement *host = ele->FirstChildElement();	
		while (host)
		{				
		
			const char* hostname = host->Attribute("Host");
			if (hostname != NULL)
			{
				try
				{
					DATA.HostName = hostname;
				}
				catch (std::regex_error&e)
				{
					std::cerr << e.code() << endl;
					return FALSE;
				}
			}
			else			
				return FALSE;

			const char* file = host->Attribute("file");
			if (file != NULL)
				DATA.filename = file;
			else
				return FALSE;

		
			const char* URI = host->Attribute("URI");
			if (URI != NULL)
				DATA.URI = URI;
			else			
				return FALSE;
			
			info_Table->push_back(DATA);
			host = host->NextSiblingElement();
		}

		// get mac address info
		ele = ele->NextSiblingElement();
		if (ele)
		{
			TiXmlElement *mac = ele->FirstChildElement();
			while (mac)
			{
				printf("the mac address is %s \n", mac->FirstChild()->Value());

				char* mp = const_cast<char*>(mac->FirstChild()->Value());

				while (*mp != '\0')
				{
					if (*mp != '-')
					{
						buf.append(mp, 1);
					}
					mp++;
				}
				MACQuery->push_back(buf);
				buf.clear();
				mac = mac->NextSiblingElement();
			}
		}
	}

	if ((info_Table = NULL) || (MACQuery = NULL))
	{
		return FALSE;
	}
	//LOG_FUNC_END(ReadXmlFile);
	return TRUE;
}

//Get adapt MAC list
const vector<string>& GetMACAddress(vector<string> *MACQuery, vector<string>& resp){

	//LOG_FUNC_BEGIN(GetMACAddress);
	CHAR AdapterName[8192];
	ULONG AdapterLength;
	DWORD dwErrorCode;
	AdapterLength = sizeof(AdapterName);
	INT i = 0;
	PPACKET_OID_DATA  OidData;
	INT AdapterNum = 0;
	LPADAPTER    lpAdapter = 0;


	if (PacketGetAdapterNames((PTSTR)AdapterName, &AdapterLength) == FALSE)
	{
		LOG("unable to retrieve the list of adapter")
		return resp;
	}

	char *move = AdapterName;
	char *startpoint = AdapterName;
	while ((*move != '\0') || (*startpoint != '\0'))
	{
		if (*move == '\0')
		{
			memcpy(AdapterNameList[i], startpoint, move - startpoint);
			i++;
			startpoint = move + 1;
		}
		move++;
	}

	AdapterNum = i;
	OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA));

	// get MAC list
	for (INT j = 0; j < i; j++)
	{
		lpAdapter = PacketOpenAdapter(AdapterNameList[j]);
		if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE))
		{
			dwErrorCode = GetLastError();
			printf("Unable to open the adapter, Error Code : %lx\n", dwErrorCode);
			return  resp;
		}

		OidData->Oid = OID_802_3_CURRENT_ADDRESS;
		OidData->Length = 6;
		ZeroMemory(OidData->Data, 6);

		if (PacketRequest(lpAdapter, FALSE, OidData))
		{

			mac_ad = bytestohexstring(OidData->Data, OidData->Length);

			MAC_List.insert(map<string, string>::value_type(AdapterNameList[j], mac_ad));
			mac_ad.clear();
		}
	}

	free(OidData);
	PacketCloseAdapter(lpAdapter);

	//MAC match
	map<string, string>::iterator MACiter;

	for (UINT i = 0; i < MACQuery->size(); i++)
	{
		transform(MACQuery->at(i).begin(), MACQuery->at(i).end(), MACQuery->at(i).begin(), towlower);
		MACiter = find_if(MAC_List.begin(), MAC_List.end(), MACMATCH(MACQuery->at(i)));
		if (MACiter != MAC_List.end())
			resp.push_back(MACiter->first);
		else
			printf("adatper : %s matched failure", MACQuery[i]);
	}

	return resp;
}

// URI match
INT URIMatch(const INT &ip_len, CHAR* ip_pkt_data,const std::regex &URI)
{	
	std::string pkt_seg(ip_pkt_data, ip_len);
	std::string uri_seg_h("GET");
	std::string uri_seg_t("\r\n");
	std::string uri_seg;
	string::iterator iteration1, iteration2;
	INT pos1, pos2;

	iteration1 = search(pkt_seg.begin(), pkt_seg.end(), uri_seg_h.begin(), uri_seg_h.end());
	if (iteration1 == pkt_seg.end())
		return 0;
	pos1 = distance(pkt_seg.begin(), iteration1);

	iteration2 = search(pkt_seg.begin() + pos1, pkt_seg.end(), uri_seg_t.begin(), uri_seg_t.end());
	if (iteration2 == pkt_seg.end())
		return 0;
	pos2 = distance(pkt_seg.begin(), iteration2);

	if (pos2 - 8 > pos1 + 4)
	{
		uri_seg.reserve(pos2 - 8 - pos1 - 4 + 1);
		uri_seg.assign(pkt_seg.begin() + pos1 +4, pkt_seg.begin() + pos2 - 8);
		std::smatch result;
		if (std::regex_search(uri_seg, result, URI))
		{
			return 	TRUE;    //URI mathed
		}
		else
		{
			return FALSE; //URI dismathed
		}
	}
	return 0;
}

void HostListMatch(XMLinfo& xmlinfo,const string NH){
	//LOG_FUNC_END(HostListMatch);

	std::pair<std::map<string, INT>::iterator, bool> res;

	xmlinfo.data_mutex.Lock();
	res = xmlinfo.HostList.insert(map<string, INT>::value_type(NH, 1));
	if (res.second == FALSE)
	{
		res.first->second++;
		//LOG2("Existing,count +1,count", res.first->second);
	}
	else
//	LOG2("New, hostname", res.first->first);

	xmlinfo.data_mutex.Unlock();

	//LOG_FUNC_END(HostListMatch);
}

//Host Name match
void HostNameRegExMatch(const INT& ip_len, CHAR* ip_pkt_data, vector<XMLinfo>*info_Table, const ip_address& ip_new)
{
	//LOG_FUNC_BEGIN(HostNameRegExMatch);
	/*CHAR buff[1024];
	INT buffsize = 0;
	bool find_host = false;
	INT i;*/

	std::string pkt_seg(ip_pkt_data, ip_len);
	std::string host_seg_h("Host");
	std::string host_seg_t("\r\n");
	std::string host_seg;
	string::iterator iteration1, iteration2;
	INT pos1, pos2;

	iteration1 = search(pkt_seg.begin(), pkt_seg.end(), host_seg_h.begin(), host_seg_h.end());
	if (iteration1 == pkt_seg.end())
		return;

	pos1 = distance(pkt_seg.begin(), iteration1);
	iteration2 = search(pkt_seg.begin() + pos1, pkt_seg.end(), host_seg_t.begin(), host_seg_t.end());
	if (iteration2 == pkt_seg.end())
		return;
	pos2 = distance(pkt_seg.begin(), iteration2);

	if (pos2 > pos1 + 8)
	{
		host_seg.reserve(pos2 - 6 - pos1 + 1);
		host_seg.assign(pkt_seg.begin() + pos1 + 6, pkt_seg.begin() + pos2);
	//	LOG2("host_seg capture ", host_seg);
		vector<XMLinfo>::iterator iter;

		for (UINT ii = 0; ii < info_Table->size(); ii++)
		{
			XMLinfo &R = info_Table->at(ii);
			if (std::regex_search(host_seg, R.HostName)) //hit host regex
			{
			//	LOG2("hit hostname regex,string", host_seg);
				if (URIMatch(ip_len, ip_pkt_data, R.URI)) // URI match	
				{
				//	LOG2("hit URI regex, string", host_seg);
					HostListMatch(R, host_seg);
				}
			}
		}
	}
	


	/*
	for (i = 0; i < ip_len; i++)
	{
		if (!find_host && i + 4 < ip_len && strncmp(ip_pkt_data + i, "Host", strlen("Host")) == 0)
		{
			find_host = true;
		}
		if (find_host)
		{
			buff[buffsize] = ip_pkt_data[i];
			buffsize++;
		}

		//end part of host segment
		if (find_host && i+2<ip_len && strncmp(ip_pkt_data + i, "\r\n", strlen("\r\n")) == 0)
		{
			find_host = false;
			buff[buffsize - 1] = { '\0' };
			char hostbuff[1500] = { '\0' };
			strncpy(hostbuff, &buff[6], buffsize - 7);
			string hoststr = hostbuff;
			LOG2("extract", hoststr);
			vector<XMLinfo>::iterator iter;

			for (INT ii = 0; ii < info_Table->size(); ii++)
			{
				XMLinfo &R = info_Table->at(ii);
				if (std::regex_search(hoststr, R.HostName)) //hit host regex
				{
					LOG2("hit hostname regex,string", hoststr);
					if (URIMatch(ip_len, ip_pkt_data, R.URI)) // URI match	
					{
						LOG2("hit URI regex, string", hoststr);
						HostListMatch(R, hoststr);
					}				
				}				
			}
			LOG(" matching fail")
			break;
		}
	}*/

// LOG_FUNC_END(HostNameRegExMatch);
}

bool Timetowrite(vector<XMLinfo>*info_Table)
{
	time(&now);
	if (((now - justnow) >= 60*10))
	{	
		for (vector <XMLinfo>::iterator k = info_Table->begin(); k != info_Table->end(); k++)
		{
			//reset everyday
			if (localtime(&now)->tm_mday != localtime(&justnow)->tm_mday)
			{
				k->HostList.erase(k->HostList.begin(), k->HostList.end());
			}

			//get time year/month/day
			INT newyear = 1900 + (localtime(&now)->tm_year);
			string Y = int2string(newyear);
			INT newmonth = 1 + localtime(&now)->tm_mon;
			string M = int2string(newmonth);
			string D = int2string(localtime(&now)->tm_mday);

	
			//get hostname
			string H = k->filename;
			
			string filename = AppPath + Y + M + D + "_" + H + ".txt";

		
			ofstream doc(filename.c_str(), ios_base::app);

			doc << CLog::GetSystemTime() << "\n\n";

			//print
			k->data_mutex.Lock();
			map<string, INT>  set = k->HostList;
			k->data_mutex.Unlock();

			if (set.size()>0)
			{
				for (map<string, INT>::iterator kk = set.begin(); kk != set.end(); kk++)
				{
				//	LOG("Recording");
					doc << kk->first << "\t\t" << kk->second << "\n\n";
				}
			}
		}
		justnow = now;
		
	}
	return  TRUE;
}
//Timer thread start
unsigned  int TimerThread(LPVOID IpParam){
	
	// first time
	time(&justnow);
	INT newyear = 1900 + (localtime(&justnow)->tm_year);
	string Y = int2string(newyear);
	INT newmonth = 1 + localtime(&justnow)->tm_mon;
	string M = int2string(newmonth);
	string D = int2string(localtime(&justnow)->tm_mday);
	vector<XMLinfo> *info_Table = (vector<XMLinfo> *)IpParam;
	
	for (vector <XMLinfo>::iterator k = info_Table->begin(); k != info_Table->end(); k++)
	{

		//get hostname
		string H = k->filename;

		string filename = AppPath  + Y + M + D + "_" + H + ".txt";

		ofstream doc(filename.c_str(), ios_base::app);
		if (!doc)
			return FALSE;

		// time stamp
		doc << CLog::GetSystemTime() << "\n\n";
	}
	while (1){
	
		Timetowrite(info_Table);
		csSleep(10);
	}

}

 //New Pcap thread Start
unsigned  int NewPcapThread(void *IpParam)
{
	
	char errbuf[PCAP_ERRBUF_SIZE];	
	GroupPrarm* groupprarm = (GroupPrarm*)IpParam;
	INT res;
	pcap_t* adhandle;
	u_int netmask;
	struct bpf_program fcode;
	int i = 0;
	struct pcap_pkthdr *pheader; /* packet header */
	const u_char * pkt_data; /* packet data */
	bool list_build = FALSE;
	string dn = groupprarm->device_name;
	vector< XMLinfo> *info_Table = groupprarm->info_Table;
	UINT threadId = groupprarm->pthreadnum;
	

	delete groupprarm;

	if ((adhandle = pcap_open(dn.c_str(),
		65536,
		PCAP_OPENFLAG_PROMISCUOUS,
		1000,
		NULL,
		errbuf
		)) == NULL)
	{
		return false;
	}
	else
	{
		printf("the device we will open is %s \n", dn.c_str());
	}


	//adhandle = pcap_open_offline("D:\\shop.pcap", errbuf);

	netmask = 0xffffff;

	//filter
	if (pcap_compile(adhandle, &fcode, "tcp dst port 80", 1, netmask) < 0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		return -1;
	}
	if (pcap_setfilter(adhandle, &fcode) < 0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		return -1;
	}
	//LOG2("thread start", threadId);

	// cap a packet
	while ((res = pcap_next_ex(adhandle, &pheader, &pkt_data)) >= 0) {

	//	LOG2("Cap a packet", threadId);
		if (res == 0)
			continue;

		ether_header *eh = (ether_header*)(pkt_data);
		IP_header *ih;
		USHORT Type = ntohs(eh->ether_typed);
		// Vlan
		if (Type == Ether_Vlan)
		{
			
			ih = (IP_header*)(pkt_data + sizeof(ether_header)+4);
		}
		else if (Type == Ether_IP) {
			ih = (IP_header*)(pkt_data + sizeof(ether_header));
		}
		else
		{
		//	LOG2("Other protocol ,return, Ether type", Type);
			continue;
		}

		INT ip_len = ntohs(ih->total_length);
		INT buffsize = 0;
		CHAR* ip_pkt_data = (CHAR*)ih;
		ip_address ip_new = ih->source_ip;
		HostNameRegExMatch(ip_len, ip_pkt_data + buffsize, info_Table, ip_new);
	}

	return -1;
}

void RecievePacketMatch(vector<string> *MACQuery, vector<XMLinfo> *info_Table)
{
	vector<string> resp;
	GroupPrarm Groupprarm;
	UINT inum;


	// time thread
	csCreateThread(TimerThread, info_Table);

	//string  AdList = csGetMacFromAdapterName();
	GetMACAddress(MACQuery, resp);;
	//map<string, string>::iterator MLiter;
	if (resp.empty())
	{
		printf("Adapter Not Found \n");
		exit(-1);
	}

	for (inum = 0; inum < resp.size(); inum++)
	{		
		GroupPrarm* Q = new GroupPrarm(Groupprarm);
		Q->device_name = resp.at(inum);
		Q->info_Table = info_Table;
		Q->pthreadnum = GetCurrentThreadId();
				// packet cap thread
		csCreateThread(NewPcapThread, Q);
		
	}
}

int main()
{
	//LOG("Main start");
	vector<string> MACQuery;


	string filename = "config.xml";
	//get the path

	justnow = 0;
	AppPath = csGetExePath();
	string FullPath = AppPath + filename;

	/*if (!BuildXmlFile(FullPath))
		return FALSE;*/

	CLog::ResetLogFile();


	if (!ReadXmlFile(FullPath, &info_Table, &MACQuery))
	{
		printf("read data error \n");
		return FALSE;
	}

	memset(bitmap, 0, (Max_pos_Num / sizeof(unsigned int)));

	RecievePacketMatch(&MACQuery, &info_Table);

	while (1){
	csSleep(2000);
	}
	

}

