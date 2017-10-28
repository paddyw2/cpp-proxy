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
        int replace_tamper(char * message);
        proxyclient get_proxy(int socket_id, vector<proxyclient> proxy_list);
        int remove_client(int client_socket, fd_set sockets, vector<proxyclient> & proxies);
        int check_proxy_responses(vector<proxyclient> proxies, fd_set active_fd_set);
        int process_replace_logging(int replace_set, int logging_set, char * arguments[]);

    private:
        int sockfd;
        int clientsockfd;
        int portno;
        int destport;
        char * serverurl;
        struct sockaddr_in serv_addr;
        struct sockaddr_in cli_addr;
        char password[BUFFERSIZE];
        int logging_option;
        int replace_option;
        char replace_str_old[128];
        char replace_str_new[128];


};
