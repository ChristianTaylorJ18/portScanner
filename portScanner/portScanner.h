#pragma once
#include "pch.h"

void listOpenPorts(const std::string& IPAddress,int startPort, int endPort, int portsPerThread);
void scanPortRange(const std::string& ipaddress, int startPort, int endPort);
