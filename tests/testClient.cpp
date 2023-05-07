#include "PipeHandler.hpp"

#include <iostream>


using namespace std;
using namespace PipeHandler;

int main()
{
    std::string pipeName = "mynamedpipe";
    Client* client=new Client(pipeName);

    Sleep(2000);

    while(1)
    {
        string out="HelloHelloHelloHelloHelloHelloHelloHelloHello";
        client->sendData(out);

        string ret;
        client->receiveData(ret);

        std::cout << "Client - " << ret << std::endl;

        Sleep(1000);    
    }

}