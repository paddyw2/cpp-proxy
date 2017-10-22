#include "server.h"

server::server(int argc, char * argv[])
{
    // check command line arguments
    if (argc < 4) {
        fprintf(stderr,"ERROR\nUsage: ./proxy [logOptions] [replaceOptions] srcPort server dstPort\n");
        exit(1);
    }

    // create client socket and check for errors
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");

    // convert argument to port number // and check for errors
    portno = atoi(argv[1]);
    destport = atoi(argv[3]);
    serverurl = argv[2];
    if(portno < 0 || destport < 0)
       error("ERROR invalid port number");

    // clear structures and set to chosen values
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // bind socket to chosen address and port
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
             error("ERROR on binding");

    // start listening for connections on the
    // created socket
    listen(sockfd,5);

}

/*
 * 
 * Runs infinitely, waiting for and handling client
 * connections one at a time
 */
int server::start_server()
{
    vector<proxyclient> client_proxies;
    struct timeval timeout;
    // set timeout to be 1 second
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    // set socket structs
    fd_set active_fd_set;
    fd_set read_fd_set;
    fd_set write_fd_set;

    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);

    // define variables
    char buffer[BUFFERSIZE];
    int error_flag;
    socklen_t clilen = sizeof(cli_addr);

    // enter infinite server loop
    while(1) {
        read_fd_set = active_fd_set;
        write_fd_set = active_fd_set;
        if(select(FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, &timeout) < 0)
            error("Select error\n");

        for(int i=0;i<FD_SETSIZE;i++) {
            if(FD_ISSET(i, &read_fd_set)) {
                if(i == sockfd) {
                    // which means a new client wants to conenct
                    cout << "Getting a client connection" << endl;
                    // accept new client connection
                    clientsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,&clilen);

                    // error check
                    if (clientsockfd < 0)
                       error("ERROR on accept");

                    char welcome[] = "Welcome to the server, type 'help' for "
                                     "a list of commands\n";
                    write_to_client(welcome, strlen(welcome), clientsockfd);
                    FD_SET(clientsockfd, &active_fd_set);
                    // create a proxy for the client and
                    // add to list
                    proxyclient new_proxy(destport, serverurl, clientsockfd);
                    client_proxies.push_back(new_proxy);
                } else {
                    // one of our existing clients are sending the
                    // server data
                    
                    // if the remote server has nothing to send
                    // back to us, we send our message to it
                    proxyclient proxy = get_proxy(i, client_proxies);
                    bzero(buffer,BUFFERSIZE);
                    int message_size = read_from_client(buffer,BUFFERSIZE-1, i);
                    cout << "Received client input: " << buffer << endl;
                    // if our local client message size is 0
                    // the client is disconnected
                    if(message_size == 0) {
                        // remove client
                        remove_client(i, active_fd_set, client_proxies);

                        cout << "Local client closed" << endl;
                    } else {
                        // if message size is normal, send to proxy
                        proxy.send_message(buffer, message_size);
                    }

                    int ready_respond = proxy.check_response_ready();

                    if(ready_respond == 1) {
                        // if the remote server is ready to send
                        // back to us, continually get its response
                        // until it doesn't have anything else to 
                        // send
                        cout << "Sending!" << endl;
                        char response[2048];
                        int length = 1;
                        while(length != 0) {
                            bzero(response, 2048);
                            length = proxy.receive_message(response, sizeof(response));
                            if(length == 0) {
                                // remote server disconnected, so disconnect
                                // client
                                FD_CLR(i, &active_fd_set);
                                remove_client(i, active_fd_set, client_proxies);
                                cout << "Remote client closed" << endl;
                            } else {
                                write_to_client(response, length, i);
                            }
                        }
                    }  
                }
            }
        }
    }
    return 0;
}

int server::remove_client(int client_socket, fd_set sockets, vector<proxyclient> proxies)
{
    close(client_socket);
    FD_CLR(client_socket, &sockets);
    proxyclient prxy = get_proxy(client_socket, proxies);
    int target_id = prxy.get_socket_origin_id();

    vector<proxyclient>::iterator itr;
    itr = proxies.begin();
    while(itr != proxies.end()) {
        if(target_id == (*itr).get_socket_origin_id())
            proxies.erase(itr);
        else
            itr++;
    }

    return 0;
}



/*
 * Writes to client socket and checks
 * for errors
 */
int server::write_to_client(char * message, int length, int client)
{
    int error_flag;
    error_flag = write(client, message, length); 
    // error check
    if (error_flag < 0)
        error("ERROR writing to socket");
    return 0;
}

/*
 * Reads from client socket and checks
 * for errors
 */
int server::read_from_client(char * message, int length, int client)
{
    int error_flag;
    error_flag = read(client, message, length); 
    //strip_newline((char *)message, length);
    // error check
    if (error_flag < 0)
        error("ERROR reading from socket");
    return error_flag;
}

/*
 * Strip new line and carriage return
 * from string
 */
int server::strip_newline(char * input, int max)
{
    if(input[strlen(input)-1] == '\n')
        input[strlen(input)-1] = 0;

    if(input[strlen(input)-1] == '\r')
        input[strlen(input)-1] = 0;

    return 0;
}

int server::replace_tamper(char * message)
{
    return 0;
}

proxyclient server::get_proxy(int socket_id, vector<proxyclient> proxy_list)
{
    vector<proxyclient>::iterator itr;
    itr = proxy_list.begin();
    while(itr != proxy_list.end()) {
        if(socket_id == (*itr).get_socket_origin_id())
            return *itr;
        else
            itr++;
    }
    error("No proxy found for the specified client socket\n");
    proxyclient null_proxy;
    return null_proxy;
}

/*
 * Error handler
 */
void server::error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
 * Closes the server
 */
int server::stop_server()
{
    cout << "Closing server socket" << endl;
    close(sockfd);
    exit(EXIT_SUCCESS);
}
