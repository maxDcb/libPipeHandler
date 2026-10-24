#include "PipeHandler.hpp"

#include <cstdint>

namespace
{
constexpr DWORD IoTimeoutMs = 30000;
constexpr std::int32_t MaxMessageBytes = 64 * 1024 * 1024;

bool completeIo(HANDLE pipe, OVERLAPPED& operation, HANDLE stopEvent, DWORD& transferred,
                DWORD timeoutMs = IoTimeoutMs)
{
    HANDLE events[] = {operation.hEvent, stopEvent};
    const DWORD result = WaitForMultipleObjects(stopEvent ? 2 : 1, events, FALSE, timeoutMs);
    if (result != WAIT_OBJECT_0)
    {
        CancelIoEx(pipe, &operation);
        WaitForSingleObject(operation.hEvent, INFINITE);
        return false;
    }
    return GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE;
}

bool transfer(HANDLE pipe, bool write, void* buffer, DWORD length, HANDLE stopEvent,
              DWORD timeoutMs = IoTimeoutMs)
{
    OVERLAPPED operation = {};
    operation.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent)
        return false;

    DWORD transferred = 0;
    const BOOL immediate = write
        ? WriteFile(pipe, buffer, length, nullptr, &operation)
        : ReadFile(pipe, buffer, length, nullptr, &operation);
    bool success = false;
    if (immediate)
        success = GetOverlappedResult(pipe, &operation, &transferred, FALSE) != FALSE;
    else if (GetLastError() == ERROR_IO_PENDING)
        success = completeIo(pipe, operation, stopEvent, transferred, timeoutMs);

    CloseHandle(operation.hEvent);
    return success && transferred == length;
}

bool sendData(HANDLE pipe, const std::string& data, HANDLE stopEvent)
{
    if (data.size() > static_cast<size_t>(MaxMessageBytes))
        return false;

    std::int32_t size = static_cast<std::int32_t>(data.size());
    if (!transfer(pipe, true, &size, sizeof(size), stopEvent))
        return false;
    return size == 0 || transfer(pipe, true, const_cast<char*>(data.data()), size, stopEvent);
}

bool receiveData(HANDLE pipe, std::string& data, HANDLE stopEvent)
{
    data.clear();
    std::int32_t size = 0;
    if (!transfer(pipe, false, &size, sizeof(size), stopEvent) ||
        size < 0 || size > MaxMessageBytes)
        return false;

    if (size == 0)
        return true;
    data.resize(size);
    if (!transfer(pipe, false, &data[0], size, stopEvent))
    {
        data.clear();
        return false;
    }
    return true;
}
}

namespace PipeHandler
{
Server::Server(const std::string& pipeName)
    : m_pipeName("\\\\.\\pipe\\" + pipeName),
      m_stopEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr))
{
}

Server::~Server()
{
    stop();
    reset();
    if (m_stopEvent)
        CloseHandle(m_stopEvent);
}

bool Server::reset()
{
    if (m_pipe != INVALID_HANDLE_VALUE)
    {
        if (m_isInit)
            DisconnectNamedPipe(m_pipe);
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    m_isInit = false;
    return true;
}

void Server::stop()
{
    if (m_stopEvent)
        SetEvent(m_stopEvent);
}

bool Server::openServer()
{
    reset();
    if (!m_stopEvent || WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0)
        return false;
    const DWORD access = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
        (m_hasOpened ? 0 : FILE_FLAG_FIRST_PIPE_INSTANCE);
    m_pipe = CreateNamedPipeA(m_pipeName.c_str(), access,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, 512, 512, 0, nullptr);
    if (m_pipe != INVALID_HANDLE_VALUE)
        m_hasOpened = true;
    return m_pipe != INVALID_HANDLE_VALUE;
}

bool Server::acceptConnection()
{
    if (m_pipe == INVALID_HANDLE_VALUE)
        return false;

    OVERLAPPED operation = {};
    operation.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!operation.hEvent)
        return false;
    const BOOL immediate = ConnectNamedPipe(m_pipe, &operation);
    bool connected = immediate != FALSE;
    if (!connected)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED)
            connected = true;
        else if (error == ERROR_IO_PENDING)
        {
            HANDLE events[] = {operation.hEvent, m_stopEvent};
            const DWORD result = WaitForMultipleObjects(2, events, FALSE, INFINITE);
            if (result == WAIT_OBJECT_0)
            {
                DWORD transferred = 0;
                connected = GetOverlappedResult(m_pipe, &operation, &transferred, FALSE) != FALSE;
            }
            else
            {
                CancelIoEx(m_pipe, &operation);
                WaitForSingleObject(operation.hEvent, INFINITE);
            }
        }
    }
    CloseHandle(operation.hEvent);
    m_isInit = connected && WaitForSingleObject(m_stopEvent, 0) != WAIT_OBJECT_0;
    return m_isInit;
}

bool Server::initServer()
{
    return openServer() && acceptConnection();
}

bool Server::sendData(std::string& data)
{
    if (!m_isInit || !::sendData(m_pipe, data, m_stopEvent))
    {
        reset();
        return false;
    }
    return true;
}

bool Server::receiveData(std::string& data)
{
    if (!m_isInit || !::receiveData(m_pipe, data, m_stopEvent))
    {
        reset();
        return false;
    }
    return true;
}

void Server::waitForClientClose()
{
    if (m_isInit)
    {
        char ignored = 0;
        // Each exchange uses one connection. Keep the response available until
        // the client has read it and closed its handle, with a bounded wait.
        transfer(m_pipe, false, &ignored, 1, m_stopEvent, 5000);
    }
}

Client::Client(const std::string& ip, const std::string& pipeName)
    : m_pipeName("\\\\" + ip + "\\pipe\\" + pipeName)
{
}

Client::~Client()
{
    reset();
}

bool Client::reset()
{
    m_isInit = false;
    if (m_pipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
    }
    return true;
}

bool Client::initConnection()
{
    reset();
    m_pipe = CreateFileA(m_pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (m_pipe == INVALID_HANDLE_VALUE)
        return false;

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(m_pipe, &mode, nullptr, nullptr))
    {
        reset();
        return false;
    }
    m_isInit = true;
    return true;
}

bool Client::closeConnection()
{
    return reset();
}

bool Client::sendData(std::string& data)
{
    if (!m_isInit || !::sendData(m_pipe, data, nullptr))
    {
        reset();
        return false;
    }
    return true;
}

bool Client::receiveData(std::string& data)
{
    if (!m_isInit || !::receiveData(m_pipe, data, nullptr))
    {
        reset();
        return false;
    }
    return true;
}
}
