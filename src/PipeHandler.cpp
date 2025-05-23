#include "PipeHandler.hpp"

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#pragma comment(lib, "ws2_32.lib")


using namespace PipeHandler;


bool _sendData(HANDLE pipe, std::string& data)
{
	BOOL fSuccess = false;
	DWORD cbWritten = 0; 

	int nbData = data.size();

	// std::cout << "WriteFile nbData " << std::to_string(nbData) << std::endl;

	fSuccess = WriteFile(pipe, &nbData, sizeof(int), &cbWritten, NULL);
	if (!fSuccess || sizeof(int) != cbWritten)
	{   
		// _tprintf(TEXT("SendData failed, GLE=%d.\n"), GetLastError()); 
		return false;
	}

	fSuccess = WriteFile(pipe, &data[0], nbData, &cbWritten, NULL);
	if (!fSuccess || cbWritten!= nbData)
	{   
		_tprintf(TEXT("SendData failed, GLE=%d.\n"), GetLastError()); 
		return false;
	}

	// std::cout << "WriteFile cbWritten " << std::to_string(cbWritten) << " cbWritten " << std::to_string(cbWritten) << std::endl;

	return true;
}


bool _receiveData(HANDLE pipe, std::string& data)
{
	DWORD  cbRead;
	int nbData = -1;

	DWORD bytesAvail = 0;
	BOOL isOK = PeekNamedPipe(pipe, NULL, 0, NULL, &bytesAvail, NULL);
	if(!isOK || bytesAvail==0)
	{
		// _tprintf(TEXT("PeekNamedPipe, bytesAvail=%d.\n"), bytesAvail);
		return true;
	}

	// std::cout << "ReadFile nbData 1" << std::endl;

	BOOL fSuccess = ReadFile(pipe, &nbData, sizeof(int), &cbRead, NULL);
	if (!fSuccess)
	{
		// _tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		return false;
	}

	// std::cout << "ReadFile nbData " << std::to_string(nbData) << std::endl;

	if (nbData > 0)
	{
		data.resize(nbData);

		fSuccess = ReadFile(pipe, (void*)data.data(), nbData, &cbRead, NULL);
		if (!fSuccess)
		{
			// _tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
			return false;
		}

		// std::cout << "ReadFile nbData " << std::to_string(nbData) << " nbDataRead " << std::to_string(cbRead) << std::endl;
	}

	return true;
}


// https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
Server::Server(const std::string& pipeName)
{
	m_pipeName="\\\\.\\pipe\\";
	m_pipeName+=pipeName;
}


Server::~Server()
{
	CloseHandle(m_pipe);
}


bool Server::reset()
{
	m_isInit = false;
	CloseHandle(m_pipe);

	return true;
}


bool Server::initServer()
{
	reset();

	m_pipe = CreateNamedPipe( m_pipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 512, 512, 0, NULL);
	if (m_pipe == INVALID_HANDLE_VALUE) 
	{
		// _tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
		return false;
	}

	while (1)
	{
		bool fConnected = ConnectNamedPipe(m_pipe, NULL);
		if (fConnected)
		{
			m_isInit=true;
			break;
		}
		Sleep(100);
	}

	return true;
}


bool Server::sendData(std::string& data)
{
	if (!m_isInit)
		return false;

	bool res = _sendData(m_pipe, data);
	if (!res)
	{
		reset();
		return false;
	}

	return true;
}


bool Server::receiveData(std::string& data)
{
	if (!m_isInit)
		return false;

	bool res = _receiveData(m_pipe, data);
	if (!res)
	{
		reset();
		return false;
	}

	return true;
}


// https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client
Client::Client(const std::string& ip, const std::string& pipeName)
{
	m_pipeName="\\\\";
	m_pipeName+=ip;
	m_pipeName+="\\pipe\\";
	m_pipeName+=pipeName;
}


Client::~Client()
{
	DisconnectNamedPipe(m_pipe);
	CloseHandle(m_pipe);
}


bool Client::initConnection()
{
	reset();
	m_pipe = CreateFile(m_pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (m_pipe == INVALID_HANDLE_VALUE) 
	{
		_tprintf( TEXT("Could not open pipe. GLE=%d\n"), GetLastError() ); 
		return false;
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL fSuccess = SetNamedPipeHandleState(m_pipe, &dwMode, NULL, NULL);
	if (!fSuccess)
	{
		_tprintf(TEXT("SetNamedPipeHandleState failed, GLE=%d.\n"), GetLastError());
		return false;
	}

	m_isInit = true;

	return true;
}


bool Client::closeConnection()
{
	m_isInit = false;
	DisconnectNamedPipe(m_pipe);
	CloseHandle(m_pipe);

	return true;
}


bool Client::reset()
{
	m_isInit = false;
	DisconnectNamedPipe(m_pipe);
	CloseHandle(m_pipe);
	
	return true;
}


bool Client::sendData(std::string& data)
{ 
	if(!m_isInit)
		return false;

	bool res = _sendData(m_pipe, data);
	if (!res)
	{
		reset();
		return false;
	}

	return true;
}


bool Client::receiveData(std::string& data)
{
	if(!m_isInit)
		return false;

	bool res = _receiveData(m_pipe, data);
	if (!res)
	{
		reset();
		return false;
	}

	return true;
}


