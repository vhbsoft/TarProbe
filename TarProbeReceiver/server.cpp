#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <algorithm>

class ReceivedPacket {
public:
	uint32_t m_index;
	uint64_t m_time;

};

class Config {
public:
	Config(const char* path) {
		std::ifstream ifs(path);
		std::string line;
		std::vector<std::string> sl;
		std::unordered_set<int> ports;
		while (std::getline(ifs, line)) {
			splitString(line, sl);
			if (sl.size() > 1) {
				if (sl[0].compare("port") == 0) {
					int p = atoi(sl[1].c_str());
					ports.insert(p);
				}
				else if (sl[0].compare("size") == 0) {
					m_size = atoi(sl[1].c_str());
				}
				else if (sl[0].compare("out") == 0) {
					m_output = sl[1];
				}
			}
		}
		ifs.close();

		m_portNumber = ports.size();
		m_ports = new uint32_t[m_portNumber];
		int index = 0;
		for (auto i : ports) {
			m_ports[index++] = static_cast<uint32_t>(i);
		}
	}

	~Config() {
		if (m_ports) {
			delete[] m_ports;
		}
	}

	uint32_t* m_ports;
	uint32_t m_portNumber;
	uint32_t m_size;
	std::string m_output;

private:
	bool isEmpty(char ch) {
		return ch <= 32;
	}

	void splitString(std::string& str, std::vector<std::string>& v) {
		v.clear();
		uint32_t s = 0;
		while (s < str.length()) {
			while (s < str.length() && isEmpty(str[s])) {
				++s;
			}
			uint32_t e = s + 1;
			while (e < str.length() && !isEmpty(str[e])) {
				++e;
			}
			std::string temp = str.substr(s, e - s);
			v.push_back(temp);
			s = e + 1;
		}
	}
};

class Packet {
public:
	Packet(uint64_t t, uint32_t id, uint32_t size) : m_time(t), m_id(id), m_size(size) {

	}

	void output(std::ofstream& fs) {
		fs << m_id << "\t" << m_port << "\t" << m_time << "\t" << m_size << "\n";
	}

	uint64_t m_time;
	uint32_t m_id;
	uint32_t m_size;
	uint32_t m_port;
};

class PortInfo {
public:
	PortInfo() : m_leftPacketSize(0), m_id(0) {

	}

	void initBuff(uint32_t size) {
		m_buff = new unsigned char[size];
		memset(m_buff, 0, size);
	}

	~PortInfo() {
		delete[] m_buff;
	}

	void outputPackets(std::ofstream& fs) {
		for (auto& p : m_packets) {
			fs << p.m_id << "\t" << m_port << "\t" << p.m_time << "\t" << p.m_size << "\n";
		}
	}

	void onPacket(uint64_t t, uint32_t count) {
		bool add = false;
		if (m_leftPacketSize == 0) {
			m_size = readInt(0);
			m_id = readInt(2);
			if (m_size > count) {
				m_leftPacketSize = m_size - count;
			}
			else {
				m_leftPacketSize = 0;
				add = true;
			}
		}
		else {
			if (m_leftPacketSize > count) {
				m_leftPacketSize -= count;
			}
			else {
				m_leftPacketSize = 0;
				add = true;
			}
		}
		if (add) {
			m_packets.push_back(Packet(t, m_id, m_size));
		}
	}

	uint32_t readInt(int index) {
		uint32_t ret = static_cast<uint32_t>(m_buff[index]);
		uint32_t i = static_cast<uint32_t>(m_buff[index + 1]);
		ret |= i << 8;
		return ret;
	}

	uint32_t m_port;
	unsigned char* m_buff;
	std::vector<Packet> m_packets;
	uint32_t m_leftPacketSize;
	uint32_t m_id;
	uint32_t m_size;
};

bool sortPacket(Packet* l, Packet* r) {
	return l->m_time < r->m_time;
}

int main()
{
	Config c("sconfig.txt");
	struct sockaddr_in* addrs = new struct sockaddr_in[c.m_portNumber];
	int* sids = new int[c.m_portNumber];
	fd_set fdSet;
	int maxSid = 0;
	PortInfo* infos = new PortInfo[c.m_portNumber];

	for (uint32_t i = 0; i < c.m_portNumber; ++i) {
		sids[i] = socket(AF_INET, SOCK_DGRAM, 0);
		if (sids[i] < 0)
		{
			std::cout << "create socket fail!" << std::endl;
			return -1;
		}
		if (sids[i] > maxSid) {
			maxSid = sids[i];
		}
		memset(&addrs[i], 0, sizeof(addrs[i]));
		addrs[i].sin_family = AF_INET;
		addrs[i].sin_addr.s_addr = htonl(INADDR_ANY);
		addrs[i].sin_port = htons(c.m_ports[i]);

		int res = bind(sids[i], (struct sockaddr *)&addrs[i], sizeof(addrs[i]));
		if (res != 0) {
			std::cout << "can't bind to port " << c.m_ports[i] << std::endl;
			return -1;
		}
		std::cout << "listen to " << c.m_ports[i] << std::endl;
		infos[i].initBuff(c.m_size);
		infos[i].m_port = c.m_ports[i];
	}
	++maxSid;

	FD_ZERO(&fdSet);

	sockaddr_in clientAddr;
	socklen_t len = sizeof(clientAddr);
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	bool hasPacket = false;
	struct timespec time;
	uint64_t startTime = 0;
	while (true)
	{
		for (uint32_t i = 0; i < c.m_portNumber; ++i) {
			FD_SET(sids[i], &fdSet);
		}
		select(maxSid, &fdSet, 0, 0, hasPacket ? &timeout : 0);
		clock_gettime(CLOCK_MONOTONIC, &time);
		uint64_t t = 0;
		if (hasPacket) {
			t = time.tv_nsec / 1000 + time.tv_sec * 1000000 - startTime;
		}
		else {
			startTime = time.tv_nsec / 1000 + time.tv_sec * 1000000;
		}
		bool anyReceive = false;
		for (uint32_t i = 0; i < c.m_portNumber; ++i) {
			if (FD_ISSET(sids[i], &fdSet)) {
				uint32_t count = recvfrom(sids[i], infos[i].m_buff, c.m_size, 0, (struct sockaddr *)&clientAddr, &len);
				infos[i].onPacket(t, count);
				hasPacket = true;
				anyReceive = true;
			}
			FD_SET(sids[i], &fdSet);
		}
		if (!anyReceive) {
			break;
		}
	}
	std::cout << "stop receiveing, output files" << std::endl;

	std::ofstream fs;
	fs.open(c.m_output.c_str(), std::ios::out | std::ios::trunc);
	if (!fs.is_open()) {
		std::cout << "Can't write file " << c.m_output << std::endl;
		return 0;
	}
	fs << "id\tport\ttime\tsize\n";
	std::vector<Packet*> packets;
	for (uint32_t i = 0; i < c.m_portNumber; ++i) {
		for (auto& p : infos[i].m_packets) {
			p.m_port = infos[i].m_port;
			packets.push_back(&p);
		}
	}
	sort(packets.begin(), packets.end(), sortPacket);
	for (size_t i = 0; i < packets.size(); ++i) {
		packets[i]->output(fs);
	}
	fs.close();
	std::cout << "Finished" << std::endl;
	return 0;
}
