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

#define BUFFERSIZE  512

using namespace std;

class proxyclient
{
    public:
        proxyclient();
        void error(const char * msg);
        int read_from_client(char * message, int length);
        int write_to_client(char * message, int length);
        int convert_hostname_ip(char * target_ip, int target_size, char * dest_url);

    private:
        struct sockaddr_in dest_addr;
        int proxy_socket;
        int dest_port;
        int error_flag;
};
