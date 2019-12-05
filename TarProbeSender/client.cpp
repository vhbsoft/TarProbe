#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <vector>
#include <arpa/inet.h>
#include <fstream>
#include <unordered_map>
#include <stdlib.h>


enum VariableType
{
	VariableType_fix,
	VariableType_var,
	VariableType_assignVar,
	VariableType_serial,
};

class Variable
{
public:
	Variable() : m_varType(VariableType_fix), m_rangeMin(0), m_rangeMax(0), m_value(0), m_varName("") {

	}

	Variable(int32_t v) :m_varType(VariableType_fix), m_rangeMin(v), m_rangeMax(v), m_value(v), m_varName("") {

	}

	~Variable() {

	}

	void parse(std::string& str) {
		m_rangeMax = 0;
		m_rangeMin = 0;
		m_value = 0;
		auto p = str.find_first_of('=', 0);
		if (p == std::string::npos) {
			if (isNumber(str, 0)) {
				m_varType = VariableType_fix;
			}
			else {
				if (str[str.length() - 1] == ']') {
					m_varType = VariableType_serial;
					m_varName = str.substr(0, str.length() - 2);
				}
				else {
					m_varType = VariableType_var;
					m_varName = str;
				}
			}
		}
		else {
			m_varType = VariableType_assignVar;
			m_varName = str.substr(0, p);
			isNumber(str, p + 1);
		}
	}

	VariableType m_varType;
	int32_t m_rangeMin;
	int32_t m_rangeMax;
	int32_t m_value;
	std::string m_varName;
	std::vector<int32_t> m_randomValues;

	void initValue() {
		if (m_rangeMin == m_rangeMax) {
			m_value = m_rangeMin;
		}
		else {
			int r = rand();
			r %= m_rangeMax + 1 - m_rangeMin;
			m_value = r + m_rangeMin;
			m_randomValues.push_back(m_value);
		}
	}

	int32_t getValue(std::unordered_map<std::string, Variable*>& varMap) {
		if (m_varType == VariableType_assignVar || m_varType == VariableType_fix) {
			initValue();
			if (m_varType == VariableType_assignVar) {
				auto it = varMap.find(m_varName);
				if (it != varMap.end()) {
					it->second->m_value = m_value;
					it->second->m_randomValues.push_back(m_value);
				}
			}
			return m_value;
		}
		auto it = varMap.find(m_varName);
		if (it == varMap.end()) {
			return 0;
		}
		if (m_varType == VariableType_serial) {
			size_t index = m_randomValues.size();
			if (index >= it->second->m_randomValues.size()) {
				index %= it->second->m_randomValues.size();
			}
			m_value = it->second->m_randomValues[index];
			m_randomValues.push_back(m_value);
			return m_value;
		}
		m_randomValues.push_back(it->second->m_value);
		return it->second->m_value;
	}

private:
	bool isNumber(std::string& str, int32_t offset) {
		int32_t end = str.length();
		bool isRange = false;
		if (str[offset] == '[') {
			++offset;
			end = str.find_first_of(']', offset);
			if (end < 0) {
				end = str.length();
			}
			isRange = true;
		}
		int32_t splitPos = -1;
		for (int32_t i = offset; i < end; ++i) {
			if (str[i] < '0' || str[i] > '9') {
				if (str[i] == ',') {
					if (isRange) {
						splitPos = i;
					}
					else {
						return false;
					}
				}
				else {
					return false;
				}
			}
		}
		if (isRange) {
			std::string temp = str.substr(offset, splitPos - offset);
			m_rangeMin = atoi(temp.c_str());
			temp = str.substr(splitPos + 1, end - splitPos - 1);
			m_rangeMax = atoi(temp.c_str());
		}
		else {
			std::string temp = str.substr(offset, end - offset);
			m_rangeMin = atoi(temp.c_str());
			m_rangeMax = m_rangeMin;
		}
		return true;
	}
};

class PacketLoop {
public:
	PacketLoop(int32_t port, int32_t packetSize, std::string host) : m_packetSize(packetSize), m_port(port), m_host(host) {

	}

	~PacketLoop() {
		for (auto l : m_loops) {
			delete l;
		}
	}

	void clear() {
		m_packetSize.m_randomValues.clear();
		m_times.m_randomValues.clear();
		m_port.m_randomValues.clear();
		for (auto l : m_loops) {
			l->clear();
		}
	}

	void visitLoop(uint32_t* packet, uint32_t* size, std::unordered_map<std::string, Variable*>& varMap) {
		uint32_t t = static_cast<uint32_t>(m_times.getValue(varMap));
		*packet = 0;
		*size = 0;
		if (m_loops.size() <= 0) {
			*packet = t;
			for (uint32_t i = 0; i < t; ++i) {
				m_port.getValue(varMap);
				*size += m_packetSize.getValue(varMap);
			}
			return;
		}
		for (uint32_t i = 0; i < t; ++i) {
			for (auto l : m_loops) {
				uint32_t p = 0;
				uint32_t s = 0;
				l->visitLoop(&p, &s, varMap);
				*packet += p;
				*size += s;
			}
		}
	}

	unsigned char* fillBuffer(unsigned char* buff, uint32_t* id, uint32_t** sizeArray, struct sockaddr*** addrList, std::unordered_map<std::string, Variable*>& varMap,
		std::unordered_map<std::string, struct sockaddr_in>& addrMap) {
		uint32_t t = static_cast<uint32_t>(m_times.getValue(varMap));
		if (m_loops.size() <= 0) {
			uint32_t* arr = *sizeArray;
			struct sockaddr** alist = *addrList;
			for (uint32_t i = 0; i < t; ++i) {
				uint32_t port = static_cast<uint32_t>(m_port.getValue(varMap));
				uint32_t s = static_cast<uint32_t>(m_packetSize.getValue(varMap));
				struct sockaddr* padd = reinterpret_cast<struct sockaddr*>(getAddr(m_host, port, addrMap));
				setInt(buff, s);
				setInt(buff + 2, *id);
				++*id;
				*arr = s;
				++arr;
				*alist = padd;
				++alist;
				buff += s;
			}
			*sizeArray = arr;
			*addrList = alist;
			return buff;
		}
		for (uint32_t i = 0; i < t; ++i) {
			for (auto l : m_loops) {
				buff = l->fillBuffer(buff, id, sizeArray, addrList, varMap, addrMap);
			}
		}
		return buff;
	}

	void output(std::ofstream& os, int32_t indent) {
		std::string s = "";
		for (int32_t i = 0; i < indent; ++i) {
			s += "\t";
		}
		os << s << "repeat ";
		outputValue(os, &m_times);
		os << s << std::endl;
		if (m_loops.size() <= 0) {
			os << s << "\tsize ";
			outputValue(os, &m_packetSize);
			os << std::endl;
			os << s << "\tport ";
			outputValue(os, &m_port);
			os << std::endl;
		}
		else {
			for (auto l : m_loops) {
				l->output(os, indent + 1);
			}
		}
		os << s << "end" << std::endl;
	}

	Variable m_times;
	Variable m_packetSize;
	Variable m_port;
	std::string m_host;

	std::vector<PacketLoop*> m_loops;

private:
	struct sockaddr_in* getAddr(const std::string& host, uint32_t port, std::unordered_map<std::string, struct sockaddr_in>& addrMap) {
		char temp[100];
		sprintf(temp, "%s:%ud", host.c_str(), port);
		auto it = addrMap.find(temp);
		if (it == addrMap.end()) {
			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_aton(host.c_str(), &addr.sin_addr);
			addrMap[temp] = addr;
		}
		it = addrMap.find(temp);
		return &(it->second);
	}

	void outputValue(std::ofstream& os, Variable* v) {
		if (v->m_randomValues.size() <= 0) {
			os << v->m_value;
		}
		else {
			os << "[";
			for (size_t i = 0; i < v->m_randomValues.size(); ++i) {
				os << v->m_randomValues[i];
				if (i < v->m_randomValues.size() - 1) {
					os << " ";
				}
			}
			os << "]";
		}
	}

	void setInt(unsigned char* buff, uint32_t v) {
		buff[0] = static_cast<unsigned char>(v & 0xff);
		buff[1] = static_cast<unsigned char>((v >> 8) & 0xff);
	}
};

class Config {
public:
	Config(const char* path) {
		m_seed = static_cast<uint32_t>(time(0));
		std::ifstream ifs(path);
		std::string line;
		std::vector<std::string> lines;
		while (std::getline(ifs, line)) {
			lines.push_back(line);
		}
		parseLoop(lines, 0, 0);
		ifs.close();
	}

	~Config() {
		for (auto pl : m_loops) {
			delete pl;
		}
		for (auto it : m_variables) {
			delete it.second;
		}
	}

	void init() {
		for (auto it : m_variables) {
			it.second->initValue();
		}
		for (auto l : m_loops) {
			l->clear();
		}
	}

	std::string m_defaultHost;
	uint32_t m_defaultPort;
	uint32_t m_defaultPacketSize;
	uint32_t m_seed;

	std::vector<PacketLoop*> m_loops;

	std::unordered_map<std::string, Variable*> m_variables;

private:
	bool isEmpty(char ch) {
		return ch <= 32;
	}

	void addVariable(std::string& name, int32_t min, int32_t max) {
		Variable* v = new Variable();
		v->m_varType = VariableType_var;
		v->m_rangeMin = min;
		v->m_rangeMax = max;
		m_variables[name] = v;
	}

	void splitString(std::string& str, std::vector<std::string>& v) {
		v.clear();
		int32_t s = 0;
		int32_t end = str.find_first_of('/');
		if (end < 0) {
			end = str.length();
		}
		while (s < end && isEmpty(str[s])) {
			++s;
		}
		int32_t e = s + 1;
		while (e < end && !isEmpty(str[e])) {
			++e;
		}
		std::string temp = str.substr(s, e - s);
		v.push_back(temp);
		s = e + 1;
		temp = "";
		while (s < end) {
			if (!isEmpty(str[s])) {
				temp += str[s];
			}
			++s;
		}
		v.push_back(temp);
	}

	uint32_t parseLoop(std::vector<std::string>& lines, uint32_t start, PacketLoop* loop) {
		std::vector<std::string> sl;
		for (uint32_t i = start; i < lines.size(); ++i) {
			std::cout << "parsing configuration line " << i + 1 << std::endl;
			splitString(lines[i], sl);
			if (sl.size() > 0) {
				if (sl[0].compare("end") == 0) {
					return i;
				}
				if (sl[0].compare("repeat") == 0) {
					PacketLoop* nl = new PacketLoop(m_defaultPort, m_defaultPacketSize, m_defaultHost);
					nl->m_times.parse(sl[1]);
					i = parseLoop(lines, i + 1, nl);
					if (loop) {
						loop->m_loops.push_back(nl);
					}
					else {
						m_loops.push_back(nl);
					}
				}
				else if (sl[0].compare("seed") == 0) {
					m_seed = static_cast<uint32_t>(atoi(sl[1].c_str()));
				}
				else if (sl[0].compare("set") == 0) {
					Variable v;
					v.parse(sl[1]);
					if (v.m_varType != VariableType_assignVar || v.m_varType != VariableType_var) {
						addVariable(v.m_varName, v.m_rangeMin, v.m_rangeMax);
					}
				}
				else if (sl[0].compare("host") == 0) {
					if (loop) {
						loop->m_host = sl[1];
					}
					else {
						m_defaultHost = sl[1];
					}
				}
				else if (sl[0].compare("port") == 0) {
					if (loop) {
						loop->m_port.parse(sl[1]);
					}
					else {
						m_defaultPort = static_cast<uint32_t>(atoi(sl[1].c_str()));
					}
				}
				else if (sl[0].compare("size") == 0) {
					if (loop) {
						loop->m_packetSize.parse(sl[1]);
					}
					else {
						m_defaultPacketSize = static_cast<uint32_t>(atoi(sl[1].c_str()));
					}
				}
			}
		}
		return lines.size();
	}
};


int main()
{
	Config c("cconfig.txt");
	srand(c.m_seed);
	if (c.m_loops.size() > 0) {
		int sid = socket(AF_INET, SOCK_DGRAM, 0);
		if (sid < 0)
		{
			std::cout << "create socket failed" << std::endl;
			return 0;
		}
		int priority = 7;
		setsockopt(sid, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority));
		uint32_t p = 0;
		uint32_t s = 0;
		c.init();
		for (auto l : c.m_loops) {
			uint32_t tempP;
			uint32_t tempS;
			l->visitLoop(&tempP, &tempS, c.m_variables);
			p += tempP;
			s += tempS;
		}
		std::cout << "total packet " << p << std::endl;
		std::cout << "total size " << s << std::endl;
		uint32_t* packetSize = new uint32_t[p];
		std::unordered_map<std::string, struct sockaddr_in> addrs;
		struct sockaddr** addrList = new struct sockaddr*[p];

		unsigned char* buff = new unsigned char[s];
		std::cout << "generating random buffer... " << std::endl;
		std::ifstream ran("/dev/urandom", std::ios::in | std::ios::binary);
		ran.read(reinterpret_cast<char*>(buff), s);
		ran.close();
		std::cout << "finish generating buffer" << std::endl;
		srand(c.m_seed);
		c.init();
		uint32_t id = 0;
		auto tb = buff;
		uint32_t* tempSize = packetSize;
		struct sockaddr** tempAddr = addrList;
		for (auto l : c.m_loops) {
			tb = l->fillBuffer(tb, &id, &tempSize, &tempAddr, c.m_variables, addrs);
		}
		tb = buff;
		for (uint32_t i = 0; i < p; ++i) {
			sendto(sid, tb, packetSize[i], 0, addrList[i], sizeof(sockaddr_in));
			tb += packetSize[i];
		}
		delete[] buff;
		delete[] packetSize;
		delete[] addrList;
	}

	std::ofstream fs;
	fs.open("output.txt", std::ios::out | std::ios::trunc);
	fs << "seed " << c.m_seed << std::endl;
	for (auto l : c.m_loops) {
		l->output(fs, 0);
	}
	fs.close();

	std::cout << "finished, see output.txt for detail." << std::endl;
	return 0;
}
