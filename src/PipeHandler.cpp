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

	std::cout << "WriteFile nbData " << std::to_string(nbData) << std::endl;

	fSuccess = WriteFile(pipe, &nbData, sizeof(int), &cbWritten, NULL);
	if (!fSuccess || sizeof(int) != cbWritten)
	{   
		_tprintf(TEXT("SendData failed, GLE=%d.\n"), GetLastError()); 
		return false;
	}

	fSuccess = WriteFile(pipe, &data[0], nbData, &cbWritten, NULL);
	if (!fSuccess || cbWritten!= nbData)
	{   
		_tprintf(TEXT("SendData failed, GLE=%d.\n"), GetLastError()); 
		return false;
	}

	std::cout << "WriteFile cbWritten " << std::to_string(cbWritten) << " cbWritten " << std::to_string(cbWritten) << std::endl;

	return true;
}


bool _receiveData(HANDLE pipe, std::string& data)
{
	DWORD  cbRead;
	int nbData = -1;

	std::cout << "ReadFile nbData 1" << std::endl;

	BOOL fSuccess = ReadFile(pipe, &nbData, sizeof(int), &cbRead, NULL);
	if (!fSuccess)
	{
		_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
		return false;
	}

	std::cout << "ReadFile nbData " << std::to_string(nbData) << std::endl;

	if (nbData > 0)
	{
		data.resize(nbData);

		fSuccess = ReadFile(pipe, (void*)data.data(), nbData, &cbRead, NULL);
		if (!fSuccess)
		{
			_tprintf(TEXT("ReadFile failed, GLE=%d.\n"), GetLastError());
			return false;
		}

		std::cout << "ReadFile nbData " << std::to_string(nbData) << " nbDataRead " << std::to_string(cbRead) << std::endl;
	}
}


// https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
Server::Server(std::string& pipeName)
{
	m_pipeName="\\\\.\\pipe\\";
	m_pipeName+=pipeName;
}


Server::~Server()
{
}


bool Server::reset()
{
	m_isInit = false;
	CloseHandle(m_pipe);

	return true;
}


bool Server::init()
{
	m_pipe = CreateNamedPipe( m_pipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 512, 512, 0, NULL);
	if (m_pipe == INVALID_HANDLE_VALUE) 
	{
		_tprintf(TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
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
		m_isInit = init();

	bool res = _receiveData(m_pipe, data);
	if (!res)
	{
		reset();
		return false;
	}

	return true;
}


// https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-client
Client::Client(std::string& pipeName)
{
	m_pipeName="\\\\.\\pipe\\";
	m_pipeName+=pipeName;

	m_isInit = init();
}


Client::~Client()
{
	CloseHandle(m_pipe);
}


bool Client::init()
{
	while (1)
	{
		m_pipe = CreateFile(m_pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (m_pipe == INVALID_HANDLE_VALUE) 
		{
			_tprintf( TEXT("Could not open pipe. GLE=%d\n"), GetLastError() ); 
			return false;
		}

		if (m_pipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 
		if (!WaitNamedPipe(m_pipeName.c_str(), 2000))
		{
			printf("Could not open pipe: 2 second wait timed out.");
			return -1;
		}
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	BOOL fSuccess = SetNamedPipeHandleState(m_pipe, &dwMode, NULL, NULL);
	if (!fSuccess)
	{
		_tprintf(TEXT("SetNamedPipeHandleState failed, GLE=%d.\n"), GetLastError());
		return false;
	}

	return true;
}


bool Client::reset()
{
	m_isInit = false;
	CloseHandle(m_pipe);
	
	return true;
}


bool Client::sendData(std::string& data)
{ 
	if(!m_isInit)
		m_isInit = init();;

	if (m_isInit)
	{
		bool res = _sendData(m_pipe, data);
		if (!res)
		{
			reset();
			return false;
		}
	}
	else
		return false;

	return true;
}


bool Client::receiveData(std::string& data)
{
	if(m_isInit)
	{
		bool res = _receiveData(m_pipe, data);
		if (!res)
		{
			reset();
			return false;
		}
	}
	else
		return false;

	return true;
}


