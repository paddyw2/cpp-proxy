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
        proxyclient(int port, char * url, int sock_id, struct sockaddr_in cli_addr);
        int set_log_replace(int logging_option, int replace_option);
        int set_replace_strings(char * replace_old, char * replace_new);
        void error(const char * msg);
        int read_from_client(char * message, int length);
        int write_to_client(char * message, int length);
        int convert_hostname_ip(char * target_ip, int target_size, char * dest_url);
        int resolve_origin_hostname(struct sockaddr_in cli_addr);
        int print_connection_info();
        int send_message(char * orig_message, int length);
        int receive_message(char ** message, int length);
        int print_log(char * message, int direct, int size);
        int log_strip_hex(char * direction, char * message, int size);
        int log_auton(char * direction, char * message, int size);
        int find_replace(char ** message, int length);
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
        char replace_str_old[128];
        char replace_str_new[128];
        int replace_not_set;
};
