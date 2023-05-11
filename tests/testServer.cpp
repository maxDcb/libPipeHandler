#include "PipeHandler.hpp"

#include <iostream>


using namespace std;
using namespace PipeHandler;

int main()
{ 
    std::string pipeName = "mynamedpipe";
    Server* server = new Server(pipeName);

    while(1)
    {
        std::cout << "InitServer" << std::endl;

        // Wait a connection
        server->initServer();

        std::cout << "Receiving" << std::endl;

        string ret;
        bool res = server->receiveData(ret);
        if(res)
        {
            std::cout << "Received" << std::endl;

            std::cout << "Server - " << ret << std::endl;

            std::cout << "Sending" << std::endl;

            string out="{}";
            res = server->sendData(out);
            if(res)
            {
                std::cout << "Sent" << std::endl;
            }
            else
                std::cout << "send failed" << std::endl;
        }
        else
            std::cout << "Receive failed" << std::endl;
  
    }
    
    delete server;

}