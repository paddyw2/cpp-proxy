#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
//#include <arpa/inet.h>

#include <vector>
#include <string>
#include <iostream>

#define SITE_URL    "http://neverssl.com/"
#define BUFFERSIZE  512
#define PORT        80

using namespace std;

class proxyclient
{
    public:
        proxyclient();
        void error(const char * msg);
        int read_from_client(char * message, int length);
        int write_to_client(char * message, int length);

    private:
        int proxy_socket;
        int dest_port;
        int error_flag;
};
