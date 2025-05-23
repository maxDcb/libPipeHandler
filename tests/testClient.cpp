#include "PipeHandler.hpp"

#include <iostream>


using namespace std;
using namespace PipeHandler;

int main()
{
    std::string ip = "127.0.0.1";
    std::string pipeName = "mynamedpipe";
    Client* client=new Client(ip, pipeName);

    while(1)
    {
        std::cout << "InitConnection" << std::endl;

        while(!client->initConnection())
        {
            Sleep(333);
        }
        std::cout << "Sending" << std::endl;

        string out="Hello";
        bool res = client->sendData(out);
        if(res)
        {
            std::cout << "Sent" << std::endl;
            std::cout << "Receiving" << std::endl;

            string ret;
            res = client->receiveData(ret);

            if(res)
            {
                std::cout << "Received" << std::endl;

                std::cout << "Client - " << ret << std::endl;
            }
            else
                std::cout << "Receive failed" << std::endl;
        }
        else
            std::cout << "Send failed" << std::endl;

        client->closeConnection();

        Sleep(1000);    
    }

}