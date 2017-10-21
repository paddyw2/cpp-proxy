#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <vector>
#include <iostream>

#include "proxyclient.h"

#define BUFFERSIZE  512

using namespace std;

class server
{
    public:
        server(int argc, char * argv[]);
        int start_server();
        int read_from_client(char * message, int length, int client);
        int write_to_client(char * message, int length, int client);
        int strip_newline(char * input, int max);
        void error(const char * msg);
        int stop_server();

    private:
        int sockfd;
        int clientsockfd;
        int portno;
        int destport;
        char * serverurl;
        struct sockaddr_in serv_addr;
        struct sockaddr_in cli_addr;
        char password[BUFFERSIZE];

};
