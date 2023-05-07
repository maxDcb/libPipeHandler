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
        string ret;
        server->receiveData(ret);

        string out = "ManManManManManManManManManManMan";
        for (int i = 0; i < 1500; i++)
            out.append("a");

        server->sendData(out);
    }

}