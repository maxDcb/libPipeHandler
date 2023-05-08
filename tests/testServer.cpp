#include "PipeHandler.hpp"

#include <iostream>


using namespace std;
using namespace PipeHandler;

int main()
{ 
    std::string pipeName = "mynamedpipe";
    Server* server = new Server(pipeName);
    server->init();

    while(1)
    {
        string ret;
        server->receiveData(ret);

        string out = "ManManManManManManManManManManMan";
        server->sendData(out);
    }

}