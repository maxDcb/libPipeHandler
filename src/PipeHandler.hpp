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
	Server(const std::string& pipeName);
	~Server();

	bool openServer();
	bool acceptConnection();
	void stop();
	bool initServer();

	bool sendData(std::string& data);
	bool receiveData(std::string& data);
	void waitForClientClose();

private:
	bool reset();
	
	bool m_isInit = false;
	bool m_hasOpened = false;

	std::string m_pipeName;
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	HANDLE m_stopEvent = nullptr;
};



class Client
{
public:
	Client(const std::string& ip, const std::string& pipeName);
	~Client();

	bool initConnection();
	bool closeConnection();	

	bool sendData(std::string& data);
	bool receiveData(std::string& data);

private:
	bool reset();
	
	bool m_isInit = false;

	std::string m_pipeName;
	HANDLE m_pipe = INVALID_HANDLE_VALUE;
};

}
