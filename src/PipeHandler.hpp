	#pragma once

#include <thread>
#include <string>
#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

namespace PipeHandler
{

class Server
{
public:
	Server(std::string& pipeName);
	~Server();

	bool reset();

	bool sendData(std::string& data);
	bool receiveData(std::string& data);

private:
	bool init();
	bool m_isInit;

	std::string m_pipeName;
	HANDLE m_pipe;
};



class Client
{
public:
	Client(std::string& pipeName);
	~Client();

	bool reset();

	bool sendData(std::string& data);
	bool receiveData(std::string& data);

private:
	bool init();
	bool m_isInit;

	std::string m_pipeName;
	HANDLE m_pipe;
};

}
