#include "portScanner.h"
std::mutex coutMutex;
std::mutex queueMutex;
std::set<int> openPortsSet;

int main() {

	std::string ipAddress;
	char fastScanMode;

	std::cout << "Enter a valid IP address to scan: ";
	std::cin >> ipAddress;

	std::cout << "Enter fast scan mode (y/n): ";
	std::cin >> fastScanMode;

	if (ipAddress.empty()) {
		std::cerr << "Invalid IP address." << std::endl;
		return 1;
	}

	auto start = std::chrono::high_resolution_clock::now();

	listOpenPorts(ipAddress, fastScanMode, 0,65535, 4);

	std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
	std::cout << "Scan completed in " << duration.count() << " seconds." << std::endl;
}

void listOpenPorts(const std::string& ipAddress, char fastScanMode, int startPort, int endPort, int portsPerThread) {

	WSAData wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
	}

	std::vector<std::thread> threads;

	if (fastScanMode == 'n' || fastScanMode == 'N') {
		for (int port = startPort; port < endPort; port += portsPerThread) { // creates threads to scan ports in range [startPort, endPort]
			int bStart = port;
			int bEnd = std::min<int>(port + portsPerThread - 1, endPort);

			threads.emplace_back([=, &ipAddress]() { // lambda function to scan ports in the range[bStart, bEnd]
				for (int i = bStart; i <= bEnd; ++i) {
					scanPort(ipAddress, i);
				}
			});

		}
	}
	else if (fastScanMode == 'y' || fastScanMode == 'Y') {
		for (int port = 0; port < top1000Ports.size(); port += portsPerThread) { // creates threads to scan ports in range [startPort, endPort]
			int bStart = port;
			int bEnd = std::min<int>(port + portsPerThread - 1, 999);

			threads.emplace_back([=, &ipAddress]() { // lambda function to scan ports in the range[bStart, bEnd]
				for (int i = bStart; i <= bEnd; ++i) {
					scanPort(ipAddress, i);
				}
			});
		}
	}

	for (auto& thread : threads) {
		if (thread.joinable()) thread.join();
	}

	for (int port : openPortsSet) {
		std::cout << "Port " << port << " is open." << std::endl;
	}

	WSACleanup();
}

void scanPort(const std::string& ipaddress, int port) {

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create the TCP socket
		u_long mode = 1;
		ioctlsocket(sock, FIONBIO, &mode); // Set socket to non-blocking mode


		DWORD timeout = 100; // 0.1 second timeout
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, ipaddress.c_str(), &addr.sin_addr);

		int result = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
		if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) { // if the connect fails and it's not a timeout error
			closesocket(sock);
			return;
		}

		fd_set writeSet;
		FD_ZERO(&writeSet); // Initialize the set (clear it as well)
		FD_SET(sock, &writeSet); // Add our socket to the set

		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000; // 0.1 second timeout

		result = select(0, nullptr, &writeSet, nullptr, &tv); // Wait for the socket to be writable 

		if (result > 0 && FD_ISSET(sock, &writeSet)) {

			int optVal;
			int optLen = sizeof(optVal);

			getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&optVal), &optLen);

			if (optVal == 0) { // If no error, the port is open
				std::lock_guard<std::mutex> lock(coutMutex);
				openPortsSet.insert(port);
			}

			else {
				closesocket(sock);
				return;
			}

			std::lock_guard<std::mutex> lock(queueMutex);
			openPortsSet.insert(port);
		}

		closesocket(sock);
}