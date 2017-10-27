#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

#include <vector>
#include <string>
#include <iostream>

#define BUFFERSIZE  512

using namespace std;

class proxyclient
{
    public:
        proxyclient();
        proxyclient(int port, char * url, int sock_id, struct sockaddr_in cli_addr, int logging_option, int replace_option);
        void error(const char * msg);
        int read_from_client(char * message, int length);
        int write_to_client(char * message, int length);
        int convert_hostname_ip(char * target_ip, int target_size, char * dest_url);
        int resolve_origin_hostname(struct sockaddr_in cli_addr);
        int print_connection_info();
        int send_message(char * message, int length);
        int receive_message(char * message, int length);
        int print_log(char * message, int direct, int size);
        int check_response_ready();
        int get_socket_origin_id();
        int destroy();

    private:
        struct sockaddr_in dest_addr;
        int proxy_socket;
        int dest_port;
        int error_flag;
        int log_flag;
        int replace_flag;
        int socket_origin_id;
        char origin_hostname[128];
        char origin_ip[64];

};
