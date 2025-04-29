#include "portScanner.h"
std::mutex coutMutex;

int main() {
	std::string ipAddress;

	std::cout << "Enter a valid IP address to scan: ";
	std::cin >> ipAddress;
	if (ipAddress.empty()) {
		std::cerr << "Invalid IP address." << std::endl;
		return 1;
	}
	listOpenPorts(ipAddress, 0, 65535, 100);


}
void listOpenPorts(const std::string& ipAddress, int startPort, int endPort, int portsPerThread) {
	std::vector<int> openPorts;
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
	}
	std::vector<std::thread> threads;

	for (int port = startPort; port < endPort; ++port) {
		int bStart = port;
		int bEnd = (((port + portsPerThread - 1) < (65535)) ? (port + portsPerThread - 1) : (65535));
		threads.emplace_back(scanPortRange, ipAddress, bStart, bEnd);
	}
	for (auto& thread : threads) {
		if (thread.joinable()) thread.join();
	}
	WSACleanup();
}
void scanPortRange(const std::string& ipaddress, int startPort, int endPort) {
	for (int port = startPort; port <= endPort; ++port) {

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (sock == INVALID_SOCKET) {
			continue; 
		}

		DWORD timeout = 250; // 1 second timeout
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, ipaddress.c_str(), &addr.sin_addr);

		int result = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

		if (result != SOCKET_ERROR) {
			std::lock_guard<std::mutex> lock(coutMutex);
			std::cout << "Port " << port << " is open." << std::endl;
		}
		
		closesocket(sock);
	}
}